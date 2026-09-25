// BSidesKC Badge - Badge-specific menu integration
// Nexus 78e62be0: touch-first UI overhaul.
//
// Half 1 built the touch primitives (theme, TapDetector, card buttons,
// back-stack) and used them to hand-draw three screens: root/idle, the
// "Badge" submenu, and the LED Brightness options screen. Everything
// else - the ~20 remaining upstream Menu/MenuNode screens, reached the
// moment you opened Marauder's own main menu - was still upstream's
// encoder-only renderer, which is what Kevin hit ("goes back to the old
// UI... does not let me select properly").
//
// Half 2 (this file, badgeMenuLoop() below) fixes that with ONE adapter
// instead of rewriting 20 screens: it renders whatever Menu the upstream
// MenuFunctions tree currently holds as our cards, and taps a card by
// calling that MenuNode's own `callable` - exactly what pressing the
// encoder button already did. Same trick covers every submenu at once,
// including ones added later, with zero changes to MenuFunctions.cpp.
//
// The Badge/LED/Battery screens from half 1 stay as bespoke code (LED
// brightness needs an options-grid, not a menu list) and keep using the
// badgeNav back-stack. The generic adapter instead reuses upstream's own
// Menu::parentMenu chain as its back-stack - that's the "back-stack" the
// ticket says to keep, just applied to a tree that already has one.

#include "configs.h"
#ifdef HAS_SCREEN

#include "hardware/badge_menu.h"
#include "MenuFunctions.h"
#include "Display.h"
#include "hardware/buzzer.h"
#include "hardware/battery_monitor.h"
#include "hardware/led_feedback.h"
#include "hardware/display_adapter.h"
#include "hardware/encoder_handler.h"
#include "hardware/input_test.h"
#include "hardware/power_manager.h"
#include "badge_ui_theme.h"
#include "touch_input.h"
#include "badge_nav.h"
#include "UI/CardKit.h"
#include "hardware/lv_disp_port.h"

extern MenuFunctions menu_function_obj;
extern Display display_obj;

#ifdef SIM_BUILD
// Test-only seam (Nexus 0f35e128): lets sim/src/sim_main.cpp capture a
// frame from inside a blocking input loop it has no other way to reach
// headless. SIM_BUILD is only ever defined by sim/build.sh - never by
// platformio.ini - so this is entirely absent from the real firmware.
extern void sim_capture_frame(const char* label);
#define SIM_FRAME(label) sim_capture_frame(label)
#else
#define SIM_FRAME(label)
#endif

static Menu* s_mainMenu = nullptr;

// ---------------------------------------------------------------------
// Chrome shared by every screen we own: header bar (title + battery),
// and a persistent Back control (fixed position, omitted on root only).
// ---------------------------------------------------------------------

static const UiRect kBackRect = {0, 0, THEME_BACK_W, THEME_BACK_H};
static const uint8_t ZONE_BACK = 250;  // reserved id, screens use 0..N for their own controls

// ---------------------------------------------------------------------
// LVGL screen (Nexus 84f4e52c: LVGL port, "better buttons"). One
// persistent lv_obj_t every screen in this file shares - they're mutually
// exclusive in time (blocking control flow, one panel), so "clear and
// rebuild" on every repaint is the same shape the old fillScreen()-based
// code used, just with LVGL objects (and its own AA font/gradient
// compositing) doing the drawing instead of raw TFT_eSPI primitives. See
// UI/CardKit.h for why this is safe to bolt onto the existing
// TapDetector/encoder control flow below unchanged.
// ---------------------------------------------------------------------

static lv_obj_t* s_lvScreen = nullptr;

static lv_obj_t* lvScreen() {
  if (!s_lvScreen) {
    lvDispInit();
    s_lvScreen = lv_obj_create(nullptr);
    lv_obj_remove_flag(s_lvScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_lvScreen, cardkit_color(THEME_BG), 0);
    lv_obj_set_style_bg_opa(s_lvScreen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_lvScreen, 0, 0);
    lv_obj_set_style_border_width(s_lvScreen, 0, 0);
    lv_scr_load(s_lvScreen);
  }
  return s_lvScreen;
}

// Clears the screen and rebuilds chrome: header band (title + battery,
// cardkit_create_header() draws the "CC13" root subtitle itself when
// isRoot) and the persistent Back control when !isRoot. Callers add their
// own cards/content after this returns, then call lvRepaintEnd().
static void lvRepaintBegin(const char* title, bool isRoot, bool backPressed) {
  lv_obj_t* screen = lvScreen();
  lv_obj_clean(screen);
  cardkit_create_header(screen, title, isRoot, batteryGetPercent());
  if (!isRoot) cardkit_create_back_button(screen, backPressed);
}

// Instant, synchronous flush - THEME_NO_ARTIFICIAL_DELAY still applies,
// LVGL just owns the compositing now instead of raw pixel pushes.
static void lvRepaintEnd() {
  lv_refr_now(nullptr);
}

// ---------------------------------------------------------------------
// Card-button primitive: full-width, >=44px tall, rounded corners actually
// drawn, gradient-filled, beveled (raised-surface read), a real inset
// pressed state, and a chevron on rows that navigate deeper. See
// UI/CardKit.h - this struct is unchanged in shape from the pre-LVGL
// CardButton so every call site below only needed a `navigable` value
// added, not a rewrite.
// ---------------------------------------------------------------------

struct CardButton {
  UiRect rect;
  const char* title;
  const char* subtitle;   // optional, may be nullptr
  bool selected;          // e.g. current option value
  bool navigable;         // chevron affordance; false for inline toggles/values
};

static lv_obj_t* drawCard(const CardButton& c, bool pressed) {
  CardSpec spec{c.rect.x, c.rect.y, c.rect.w, c.rect.h, c.title, c.subtitle, c.selected, pressed, c.navigable};
  return cardkit_create_card(lvScreen(), spec);
}

// ---------------------------------------------------------------------
// Forward decls for the screens in this file.
// ---------------------------------------------------------------------

static void drawBadgeSubmenu();
#ifdef SIM_BUILD
// External linkage only under SIM_BUILD, so sim_main.cpp can drive this
// screen directly - it sits behind a blocking parent loop (drawBadgeSubmenu)
// the harness has no other seam into. Real firmware keeps it internal.
void ledBrightnessOptionsScreen();
#else
static void ledBrightnessOptionsScreen();
#endif
static void showBatteryStatus();
static void toggleBuzzerMute();
static void runHwTest();

// ---------------------------------------------------------------------
// Generic adapter (half 2): renders ANY upstream Menu - mainMenu itself
// (req #2: root shows top-level items directly, no "Menu" tap-through)
// and every submenu under it - as our cards, and drives selection off
// both touch and the encoder. See the file header for the design.
// ---------------------------------------------------------------------

#define MAX_ADAPTED_NODES 32  // headroom over the biggest upstream submenu (wifiSnifferMenu, ~22)

static Menu*   s_adaptedMenu = nullptr;               // Menu this adapter last drew
static bool    s_needsRebuild = true;                 // force a fresh card render next frame
static bool    s_wasInMenu = false;                   // detects returning from a scan/attack screen
static uint8_t s_bodyRaw[MAX_ADAPTED_NODES];           // raw menu->list indices, "Back" node excluded
static int     s_bodyCount = 0;
static int     s_selRow = 0;                          // index into s_bodyRaw - shared by touch + encoder
static int     s_pageStart = 0;
static int     s_pressedRow = -1;                     // live touch-down visual, -1 = none
static bool    s_backPressed = false;
static TapDetector s_adaptedTap;

// Upstream prepends a MenuNode named "Back" to every non-root submenu
// (its callable does exactly `changeMenu(parentMenu, true)`). We give
// every non-root screen our own persistent Back control in chrome
// instead, so skip upstream's copy here rather than showing it twice.
static bool isBackNode(const MenuNode& n) {
  return n.name.equals("Back");
}

static void adaptedRebuildBody(Menu* menu) {
  s_bodyCount = 0;
  if (menu->list != nullptr) {
    int n = menu->list->size();
    for (int i = 0; i < n && s_bodyCount < MAX_ADAPTED_NODES; i++) {
      if (!isBackNode(menu->list->get(i))) s_bodyRaw[s_bodyCount++] = (uint8_t)i;
    }
  }
  s_selRow = 0;
  s_pageStart = 0;
  s_pressedRow = -1;
  s_backPressed = false;
}

// How many card rows fit below the chrome. Derived from the theme, not
// hand-picked per menu - a menu with more items than this just pages
// (encoder scrolls it into view; see the "N-M of T" indicator below).
static int adaptedMaxVisible(bool isRoot) {
  uint16_t top = THEME_HEADER_H + (isRoot ? THEME_ROOT_BRAND_H : 0) + THEME_SPACE_MD;
  uint16_t bottom = THEME_SPACE_MD;
  int avail = (int)TFT_HEIGHT - (int)top - (int)bottom;
  int n = (avail + THEME_SPACE_SM) / (THEME_CARD_MIN_H + THEME_SPACE_SM);
  return n < 1 ? 1 : n;
}

static UiRect adaptedRowRect(int visualRow, bool isRoot) {
  uint16_t top = THEME_HEADER_H + (isRoot ? THEME_ROOT_BRAND_H : 0) + THEME_SPACE_MD;
  uint16_t x = THEME_SPACE_MD;
  uint16_t w = TFT_WIDTH - THEME_SPACE_MD * 2;
  uint16_t y = top + visualRow * (THEME_CARD_MIN_H + THEME_SPACE_SM);
  return {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)THEME_CARD_MIN_H};
}

static void adaptedEnsurePageVisible(int maxVisible) {
  if (s_selRow < s_pageStart) s_pageStart = s_selRow;
  if (s_selRow >= s_pageStart + maxVisible) s_pageStart = s_selRow - maxVisible + 1;
  if (s_pageStart < 0) s_pageStart = 0;
  int maxStart = s_bodyCount - maxVisible;
  if (maxStart < 0) maxStart = 0;
  if (s_pageStart > maxStart) s_pageStart = maxStart;
}

static void adaptedRender(Menu* menu, bool isRoot, int pressedRow, bool backPressed) {
  const char* title = isRoot ? "BADGE PIRATES" : menu->name.c_str();
  lvRepaintBegin(title, isRoot, backPressed);

  int maxVisible = adaptedMaxVisible(isRoot);
  adaptedEnsurePageVisible(maxVisible);

  int shown = min(maxVisible, s_bodyCount - s_pageStart);
  for (int row = 0; row < shown; row++) {
    int bodyIdx = s_pageStart + row;
    MenuNode node = menu->list->get(s_bodyRaw[bodyIdx]);
    UiRect r = adaptedRowRect(row, isRoot);
    // navigable=true: every row here is an upstream Menu/MenuNode this
    // adapter reaches via changeMenu()/callable() - always "goes
    // somewhere else", never a static label (Nexus 84f4e52c's chevron
    // requirement).
    CardButton c{r, node.name.c_str(), nullptr, bodyIdx == s_selRow, true};
    drawCard(c, pressedRow == bodyIdx);
  }

  if (s_bodyCount == 0) {
    cardkit_create_hint(lvScreen(), "Nothing here yet", TFT_WIDTH / 2, adaptedRowRect(0, isRoot).y + 10);
  } else if (s_bodyCount > maxVisible) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d-%d of %d", s_pageStart + 1, s_pageStart + shown, s_bodyCount);
    cardkit_create_hint(lvScreen(), buf, TFT_WIDTH / 2, TFT_HEIGHT - THEME_SPACE_MD - 8);
  }

  lvRepaintEnd();
}

// Fires whenever the currently-selected node's action should run - tap
// on its card, or encoder button press (req #4: both inputs, same
// outcome). Whatever the callable does to current_menu (change it,
// leave it, or take the display away entirely into a scan screen), our
// cached state is now stale, so force a full resync next frame instead
// of guessing what changed.
static void adaptedActivate(Menu* menu, int bodyRow) {
  if (bodyRow < 0 || bodyRow >= s_bodyCount) return;
  powerManagerResetActivity();
  buzzerPlay(TONE_BUTTON_PRESS);
  MenuNode node = menu->list->get(s_bodyRaw[bodyRow]);
  s_needsRebuild = true;
  if (node.callable) node.callable();
}

static void adaptedGoBack(Menu* menu) {
  if (menu->parentMenu == nullptr) return;  // root: chrome never draws a Back control here
  powerManagerResetActivity();
  buzzerPlay(TONE_BUTTON_PRESS);
  s_needsRebuild = true;
  menu_function_obj.changeMenu(menu->parentMenu, true);
}

// adaptedActivate()/adaptedGoBack() above both call into upstream code
// (MenuNode::callable, MenuFunctions::changeMenu) that draws upstream's OWN
// button-list UI as a side effect (changeMenu() always runs buildButtons()+
// displayCurrentMenu() - see MenuFunctions.cpp:3587-3735 - regardless of who
// called it). Deferring our own repaint to "next tick" via s_needsRebuild
// left that upstream draw as the last thing on the panel for a full loop()
// iteration on every single navigation - the "cards overlay upstream menu"
// flash Kevin hit on hardware, reproduced headless in the simulator (Nexus
// 0f35e128, sim/README.md). Repainting immediately, in the same call that
// triggered the change, means upstream's draw is never the last thing
// shown - not "fixed next frame", never drawn as a visible frame at all.
static void adaptedResyncNow() {
  if (!badgeMenuOwnsScreen()) return;  // the callable took us into a scan/attack screen; not ours to draw
  Menu* now = menu_function_obj.current_menu;
  if (now == nullptr) return;
  s_adaptedMenu = now;
  adaptedRebuildBody(now);
  adaptedRender(now, now->parentMenu == nullptr, -1, false);
  s_needsRebuild = false;
}

bool badgeMenuOwnsScreen() {
  return (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
         (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
         (wifi_scan_obj.currentScanMode == OTA_UPDATE);
}

void badgeMenuLoop() {
  bool inMenu = badgeMenuOwnsScreen();
  if (!inMenu) {
    s_wasInMenu = false;
    return;  // a scan/attack screen owns the display; not our concern this half
  }
  // Coming back from a scan/attack screen leaves whatever that screen drew
  // on the panel - force a redraw even though current_menu never changed.
  if (!s_wasInMenu) s_needsRebuild = true;
  s_wasInMenu = true;

  Menu* menu = menu_function_obj.current_menu;
  if (menu == nullptr) return;
  bool isRoot = (menu->parentMenu == nullptr);

  if (s_needsRebuild || menu != s_adaptedMenu) {
    s_adaptedMenu = menu;
    adaptedRebuildBody(menu);
    adaptedRender(menu, isRoot, -1, false);
    s_needsRebuild = false;
    return;  // don't also read input the same frame we just (re)drew
  }

  int maxVisible = adaptedMaxVisible(isRoot);

  // --- Encoder: secondary input, drives the same selection as touch ---
  bool encMoved = false;
  if (s_bodyCount > 0 && encoder_turned_up()) {
    s_selRow = (s_selRow == 0) ? s_bodyCount - 1 : s_selRow - 1;
    encMoved = true;
  }
  if (s_bodyCount > 0 && encoder_turned_down()) {
    s_selRow = (s_selRow >= s_bodyCount - 1) ? 0 : s_selRow + 1;
    encMoved = true;
  }
  if (encMoved) {
    powerManagerResetActivity();
    adaptedRender(menu, isRoot, -1, false);
    return;
  }
  if (encoder_button_pressed()) {
    adaptedActivate(menu, s_selRow);
    adaptedResyncNow();
    return;
  }

  // --- Touch: primary input ---
  TouchZone zones[MAX_ADAPTED_NODES + 1];
  int n = 0;
  if (!isRoot) zones[n++] = {kBackRect, ZONE_BACK};
  int shown = min(maxVisible, s_bodyCount - s_pageStart);
  for (int row = 0; row < shown; row++) {
    zones[n++] = {adaptedRowRect(row, isRoot), (uint8_t)(s_pageStart + row)};
  }

  int fired = s_adaptedTap.poll(zones, n);

  int pressedRow = -1;
  for (int row = 0; row < shown; row++) {
    int bodyIdx = s_pageStart + row;
    if (s_adaptedTap.isPressed((uint8_t)bodyIdx)) pressedRow = bodyIdx;
  }
  bool backPressed = !isRoot && s_adaptedTap.isPressed(ZONE_BACK);
  if (pressedRow != s_pressedRow || backPressed != s_backPressed) {
    s_pressedRow = pressedRow;
    s_backPressed = backPressed;
    adaptedRender(menu, isRoot, pressedRow, backPressed);
  }

  if (fired == ZONE_BACK) {
    adaptedGoBack(menu);
    adaptedResyncNow();
  } else if (fired >= 0 && fired < s_bodyCount) {
    s_selRow = fired;
    adaptedActivate(menu, fired);
    adaptedResyncNow();
  }
}

// ---------------------------------------------------------------------
// "Badge" submenu - one full-width card per item, Back always present.
// ---------------------------------------------------------------------

static void drawBadgeSubmenu() {
  badgeNavPush(drawBadgeSubmenu, "Badge");

  enum { Z_LED = 1, Z_BUZZ, Z_BATT, Z_HWTEST };

#ifndef PRODUCTION_BUILD
  const int ROWS = 4;
#else
  const int ROWS = 3;
#endif

  auto render = [&](int pressedZone) {
    lvRepaintBegin("Badge", /*isRoot=*/false, pressedZone == ZONE_BACK);
    uint16_t y = THEME_HEADER_H + THEME_SPACE_MD;
    uint16_t rowH = THEME_CARD_MIN_H;
    uint16_t x = THEME_SPACE_MD;
    uint16_t w = TFT_WIDTH - THEME_SPACE_MD * 2;

    CardButton led{{x, y, w, rowH}, "LED Brightness", "Tap to adjust", false, true};
    drawCard(led, pressedZone == Z_LED);
    y += rowH + THEME_SPACE_SM;

    // Buzzer toggles in place (no navigation), so no chevron - visually
    // distinct from the rows next to it that do go somewhere else.
    CardButton buzz{{x, y, w, rowH}, "Buzzer", buzzerIsMuted() ? "Muted" : "On", false, false};
    drawCard(buzz, pressedZone == Z_BUZZ);
    y += rowH + THEME_SPACE_SM;

    CardButton batt{{x, y, w, rowH}, "Battery Status", nullptr, false, true};
    drawCard(batt, pressedZone == Z_BATT);
    y += rowH + THEME_SPACE_SM;

#ifndef PRODUCTION_BUILD
    CardButton hw{{x, y, w, rowH}, "Hardware Test", nullptr, false, true};
    drawCard(hw, pressedZone == Z_HWTEST);
#endif
    lvRepaintEnd();
  };

  render(0);

  TapDetector tap;
  int lastPressed = 0;
  while (true) {
    uint16_t y = THEME_HEADER_H + THEME_SPACE_MD;
    uint16_t rowH = THEME_CARD_MIN_H;
    uint16_t x = THEME_SPACE_MD;
    uint16_t w = TFT_WIDTH - THEME_SPACE_MD * 2;

    TouchZone zones[5];
    int n = 0;
    zones[n++] = {kBackRect, ZONE_BACK};
    zones[n++] = {{x, y, w, rowH}, Z_LED}; y += rowH + THEME_SPACE_SM;
    zones[n++] = {{x, y, w, rowH}, Z_BUZZ}; y += rowH + THEME_SPACE_SM;
    zones[n++] = {{x, y, w, rowH}, Z_BATT}; y += rowH + THEME_SPACE_SM;
#ifndef PRODUCTION_BUILD
    zones[n++] = {{x, y, w, rowH}, Z_HWTEST};
#endif

    int fired = tap.poll(zones, n);
    int pressedNow = 0;
    for (int i = 0; i < n; i++) if (tap.isPressed(zones[i].id)) pressedNow = zones[i].id;
    if (pressedNow != lastPressed) {
      render(pressedNow);
      lastPressed = pressedNow;
    }

    if (fired == ZONE_BACK || encoder_button_pressed()) break;
    if (fired == Z_LED) { ledBrightnessOptionsScreen(); render(0); lastPressed = 0; }
    if (fired == Z_BUZZ) { toggleBuzzerMute(); render(0); lastPressed = 0; }
    if (fired == Z_BATT) { showBatteryStatus(); render(0); lastPressed = 0; }
#ifndef PRODUCTION_BUILD
    if (fired == Z_HWTEST) { runHwTest(); render(0); lastPressed = 0; }
#endif
    delay(30);
  }

  badgeNavPop();
}

// ---------------------------------------------------------------------
// LED Brightness - options-as-buttons (req #3): discrete levels as a
// row of tappable buttons, current value visibly selected. Encoder still
// works as a secondary input (rotate to move selection).
// ---------------------------------------------------------------------

#ifdef SIM_BUILD
void ledBrightnessOptionsScreen() {
#else
static void ledBrightnessOptionsScreen() {
#endif
  badgeNavPush(ledBrightnessOptionsScreen, "LED Brightness");

  const uint8_t levels[] = {5, 15, 33, 66, 100, 150, 200, 255};
  const uint8_t numLevels = 8;
  static uint8_t idx = 2;  // default ~33, persists across visits this session

  const uint16_t gridX = THEME_SPACE_MD;
  const uint16_t gridY = THEME_HEADER_H + THEME_SPACE_LG;
  const uint16_t cols = 4;
  const uint16_t cellW = (TFT_WIDTH - THEME_SPACE_MD * 2 - THEME_SPACE_SM * (cols - 1)) / cols;
  const uint16_t cellH = THEME_CARD_MIN_H;

  auto cellRect = [&](uint8_t i) -> UiRect {
    uint16_t col = i % cols, row = i / cols;
    return {(int16_t)(gridX + col * (cellW + THEME_SPACE_SM)),
            (int16_t)(gridY + row * (cellH + THEME_SPACE_SM)),
            (int16_t)cellW, (int16_t)cellH};
  };

  auto render = [&](int pressedIdx, bool backPressed) {
    lvRepaintBegin("LED Brightness", /*isRoot=*/false, backPressed);
    for (uint8_t i = 0; i < numLevels; i++) {
      UiRect r = cellRect(i);
      char buf[6];
      snprintf(buf, sizeof(buf), "%d%%", levels[i] * 100 / 255);
      // title=nullptr: these are small square option cells, not full-width
      // rows - the caller (here) centers its own label instead of
      // CardKit's default left-aligned title. No chevron: tapping a cell
      // selects a value in place, it doesn't navigate anywhere.
      CardButton c{r, nullptr, nullptr, (i == idx), false};
      lv_obj_t* cell = drawCard(c, pressedIdx == i);
      lv_obj_t* lbl = lv_label_create(cell);
      lv_label_set_text(lbl, buf);
      lv_obj_set_style_text_color(lbl, cardkit_color((i == idx) ? THEME_ACCENT : THEME_TEXT), 0);
      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
      lv_obj_center(lbl);
    }
    cardkit_create_hint(lvScreen(), "Tap a level to apply", TFT_WIDTH / 2, TFT_HEIGHT - THEME_SPACE_LG);
    lvRepaintEnd();
  };

  render(-1, false);
  led_feedback_set_brightness(levels[idx]);
  SIM_FRAME("led_brightness");

  TapDetector tap;
  int lastPressed = -1;
  bool lastBackPressed = false;
  while (true) {
    TouchZone zones[9];
    zones[0] = {kBackRect, ZONE_BACK};
    for (uint8_t i = 0; i < numLevels; i++) zones[i + 1] = {cellRect(i), i};

    int fired = tap.poll(zones, numLevels + 1);
    int pressedNow = -1;
    for (uint8_t i = 0; i < numLevels; i++) if (tap.isPressed(i)) pressedNow = i;
    bool backPressed = tap.isPressed(ZONE_BACK);

    if (pressedNow != lastPressed || backPressed != lastBackPressed) {
      render(pressedNow, backPressed);
      lastPressed = pressedNow;
      lastBackPressed = backPressed;
    }

    if (fired == ZONE_BACK) break;
    if (fired >= 0 && fired < numLevels) {
      idx = fired;
      led_feedback_set_brightness(levels[idx]);
      render(-1, false);
    }

    if (encoder_turned_up() && idx > 0) { idx--; led_feedback_set_brightness(levels[idx]); render(-1, false); }
    if (encoder_turned_down() && idx < numLevels - 1) { idx++; led_feedback_set_brightness(levels[idx]); render(-1, false); }
    if (encoder_button_pressed()) break;

    delay(30);
  }

  badgeNavPop();
}

// ---------------------------------------------------------------------
// Battery Status / Buzzer toggle / HW test - not full-restyle targets
// this half, but they're reachable from the touch-first Badge submenu
// now, so they must not be touch dead-ends (req #4). Minimal fix: honor
// a tap-anywhere-to-return alongside the existing encoder/button exit.
// ---------------------------------------------------------------------

static void showBatteryStatus() {
  display_obj.clearScreen();
  badge_display.drawCenteredTitle("Battery Status");

  int8_t pct = batteryGetPercent();
  uint16_t color = (pct > 50) ? UI_COLOR_OK : (pct > 20) ? UI_COLOR_WARN : UI_COLOR_ERR;
  display_obj.tft.setTextColor(color, UI_COLOR_BG);
  display_obj.tft.drawCentreString(String(pct) + "%", TFT_WIDTH / 2, 70, 4);

  badge_display.drawProgressBar(UI_BAR_MARGIN, 130, TFT_WIDTH - UI_BAR_MARGIN * 2, UI_BAR_H, max(0, (int)pct), color);
  badge_display.drawStatusHint("Tap anywhere or press knob to return");

  while (true) {
    uint16_t tx, ty;
    if (touchRead(&tx, &ty)) break;
    if (encoder_button_pressed()) break;
    #ifdef HAS_BUTTONS
      if (digitalRead(D_BTN) == LOW) break;
    #endif
    delay(50);
  }
}

static void toggleBuzzerMute() {
  buzzerMute(!buzzerIsMuted());
  display_obj.clearScreen();
  badge_display.drawCenteredTitle(buzzerIsMuted() ? "Buzzer: MUTED" : "Buzzer: ON");
  delay(500);  // brief confirmation flash only, not a wait-for-input screen
}

static void runHwTest() {
  runInputValidationTest();
  display_obj.clearScreen();
}

// ---------------------------------------------------------------------
// Setup: register "Badge" in upstream's main menu (one MenuNode, same as
// before) but point it at our own drawBadgeSubmenu() instead of handing
// off to upstream's Menu/MenuNode renderer. Finish by pointing
// current_menu at mainMenu - badgeMenuLoop() (called every loop()
// iteration from main.cpp) takes it from there and draws it as cards
// from the very first frame, so root IS the top-level menu (req #2),
// not a screen that hands off to one.
// ---------------------------------------------------------------------

void badgeMenuSetup() {
  Menu* mainMenu = menu_function_obj.current_menu;
  s_mainMenu = mainMenu;

  TFT_eSPI_Button* btnMain = new TFT_eSPI_Button();
  MenuNode rebootNode = mainMenu->list->get(mainMenu->list->size() - 1);
  mainMenu->list->remove(mainMenu->list->size() - 1);
  mainMenu->list->add(MenuNode{"Badge", false, TFTMAGENTA, DEVICE, btnMain, false, []() {
    drawBadgeSubmenu();
    menu_function_obj.changeMenu(s_mainMenu, true);
  }});
  TFT_eSPI_Button* btnReboot = new TFT_eSPI_Button();
  rebootNode.button = btnReboot;
  mainMenu->list->add(rebootNode);

  Serial.println(F("[Badge] Menu items added"));

  menu_function_obj.changeMenu(s_mainMenu, true);
}

#endif
