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

static void drawBattery() {
  int8_t pct = batteryGetPercent();
  uint16_t x = TFT_WIDTH - THEME_BATT_W - THEME_SPACE_SM;
  uint16_t y = (THEME_HEADER_H - THEME_BATT_H) / 2;
  uint16_t color = (pct > 50) ? THEME_OK : (pct > 20) ? THEME_WARN : THEME_ERROR;

  display_obj.tft.drawRoundRect(x, y, THEME_BATT_W, THEME_BATT_H, 2, THEME_TEXT_MUTED);
  display_obj.tft.fillRect(x + THEME_BATT_W, y + 3, 2, THEME_BATT_H - 6, THEME_TEXT_MUTED);
  uint16_t fillW = (uint16_t)((uint32_t)(THEME_BATT_W - 4) * max((int8_t)0, pct) / 100);
  display_obj.tft.fillRect(x + 2, y + 2, fillW, THEME_BATT_H - 4, color);
  if (fillW < (uint16_t)(THEME_BATT_W - 4))
    display_obj.tft.fillRect(x + 2 + fillW, y + 2, THEME_BATT_W - 4 - fillW, THEME_BATT_H - 4, THEME_BG);
}

static void drawBackButton(bool pressed) {
  uint16_t fill = pressed ? THEME_SURFACE_HI : THEME_SURFACE;
  display_obj.tft.fillRoundRect(kBackRect.x + 2, kBackRect.y + 2, kBackRect.w - 4, kBackRect.h - 4, THEME_RADIUS_SM, fill);
  display_obj.tft.drawRoundRect(kBackRect.x + 2, kBackRect.y + 2, kBackRect.w - 4, kBackRect.h - 4, THEME_RADIUS_SM, THEME_BORDER);
  display_obj.tft.setTextColor(THEME_TEXT, fill);
  display_obj.tft.drawCentreString("< Back", kBackRect.x + kBackRect.w / 2, kBackRect.y + 6, THEME_FONT_SM);
}

// Draws the header bar (fills its own band, callers draw body below it).
// isRoot == true omits the Back control (root is the one screen you can't
// back out of further).
static void drawChrome(const char* title, bool isRoot) {
  display_obj.tft.fillRect(0, 0, TFT_WIDTH, THEME_HEADER_H, THEME_BG);
  display_obj.tft.drawFastHLine(0, THEME_HEADER_H, TFT_WIDTH, THEME_BORDER);
  display_obj.tft.setTextColor(THEME_TEXT, THEME_BG);
  display_obj.tft.drawCentreString(title, TFT_WIDTH / 2, 6, THEME_FONT_MD);
  drawBattery();
  if (!isRoot) drawBackButton(false);
}

// ---------------------------------------------------------------------
// Card-button primitive: full-width, >=44px tall, rounded, pressed-state
// on touch-down, spaced enough that a fingertip can't straddle two.
// ---------------------------------------------------------------------

struct CardButton {
  UiRect rect;
  const char* title;
  const char* subtitle;   // optional, may be nullptr
  bool selected;          // e.g. current option value
};

static void drawCard(const CardButton& c, bool pressed) {
  uint16_t fill = pressed ? THEME_SURFACE_HI : THEME_SURFACE;
  uint16_t border = c.selected ? THEME_ACCENT : THEME_BORDER;
  display_obj.tft.fillRoundRect(c.rect.x, c.rect.y, c.rect.w, c.rect.h, THEME_RADIUS_MD, fill);
  display_obj.tft.drawRoundRect(c.rect.x, c.rect.y, c.rect.w, c.rect.h, THEME_RADIUS_MD, border);
  display_obj.tft.setTextColor(c.selected ? THEME_ACCENT : THEME_TEXT, fill);
  display_obj.tft.drawString(c.title, c.rect.x + THEME_SPACE_MD, c.rect.y + THEME_SPACE_XS, THEME_FONT_MD);
  if (c.subtitle) {
    display_obj.tft.setTextColor(THEME_TEXT_MUTED, fill);
    display_obj.tft.drawString(c.subtitle, c.rect.x + THEME_SPACE_MD, c.rect.y + THEME_SPACE_XS + 18, THEME_FONT_SM);
  }
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
  display_obj.tft.fillScreen(THEME_BG);

  const char* title = isRoot ? "BADGE PIRATES" : menu->name.c_str();
  drawChrome(title, isRoot);
  if (isRoot) {
    display_obj.tft.setTextColor(THEME_TEXT_MUTED, THEME_BG);
    display_obj.tft.drawCentreString("CC13", TFT_WIDTH / 2, THEME_HEADER_H + 2, THEME_FONT_SM);
  } else {
    drawBackButton(backPressed);
  }

  int maxVisible = adaptedMaxVisible(isRoot);
  adaptedEnsurePageVisible(maxVisible);

  int shown = min(maxVisible, s_bodyCount - s_pageStart);
  for (int row = 0; row < shown; row++) {
    int bodyIdx = s_pageStart + row;
    MenuNode node = menu->list->get(s_bodyRaw[bodyIdx]);
    UiRect r = adaptedRowRect(row, isRoot);
    CardButton c{r, node.name.c_str(), nullptr, bodyIdx == s_selRow};
    drawCard(c, pressedRow == bodyIdx);
  }

  if (s_bodyCount == 0) {
    display_obj.tft.setTextColor(THEME_TEXT_MUTED, THEME_BG);
    display_obj.tft.drawCentreString("Nothing here yet", TFT_WIDTH / 2, adaptedRowRect(0, isRoot).y + 10, THEME_FONT_SM);
  } else if (s_bodyCount > maxVisible) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d-%d of %d", s_pageStart + 1, s_pageStart + shown, s_bodyCount);
    display_obj.tft.setTextColor(THEME_TEXT_MUTED, THEME_BG);
    display_obj.tft.drawCentreString(buf, TFT_WIDTH / 2, TFT_HEIGHT - THEME_SPACE_MD - 8, THEME_FONT_SM);
  }
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
    display_obj.tft.fillScreen(THEME_BG);
    drawChrome("Badge", /*isRoot=*/false);
    uint16_t y = THEME_HEADER_H + THEME_SPACE_MD;
    uint16_t rowH = THEME_CARD_MIN_H;
    uint16_t x = THEME_SPACE_MD;
    uint16_t w = TFT_WIDTH - THEME_SPACE_MD * 2;

    CardButton led{{x, y, w, rowH}, "LED Brightness", "Tap to adjust", false};
    drawCard(led, pressedZone == Z_LED);
    y += rowH + THEME_SPACE_SM;

    CardButton buzz{{x, y, w, rowH}, "Buzzer", buzzerIsMuted() ? "Muted" : "On", false};
    drawCard(buzz, pressedZone == Z_BUZZ);
    y += rowH + THEME_SPACE_SM;

    CardButton batt{{x, y, w, rowH}, "Battery Status", nullptr, false};
    drawCard(batt, pressedZone == Z_BATT);
    y += rowH + THEME_SPACE_SM;

#ifndef PRODUCTION_BUILD
    CardButton hw{{x, y, w, rowH}, "Hardware Test", nullptr, false};
    drawCard(hw, pressedZone == Z_HWTEST);
#endif
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
      if (pressedNow == ZONE_BACK || lastPressed == ZONE_BACK) drawBackButton(pressedNow == ZONE_BACK);
      else render(pressedNow);
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

  auto render = [&](int pressedIdx) {
    display_obj.tft.fillScreen(THEME_BG);
    drawChrome("LED Brightness", /*isRoot=*/false);
    for (uint8_t i = 0; i < numLevels; i++) {
      UiRect r = cellRect(i);
      uint16_t fill = (pressedIdx == i) ? THEME_SURFACE_HI : THEME_SURFACE;
      uint16_t border = (i == idx) ? THEME_ACCENT : THEME_BORDER;
      display_obj.tft.fillRoundRect(r.x, r.y, r.w, r.h, THEME_RADIUS_SM, fill);
      display_obj.tft.drawRoundRect(r.x, r.y, r.w, r.h, THEME_RADIUS_SM, border);
      display_obj.tft.setTextColor((i == idx) ? THEME_ACCENT : THEME_TEXT, fill);
      char buf[6];
      snprintf(buf, sizeof(buf), "%d%%", levels[i] * 100 / 255);
      display_obj.tft.drawCentreString(buf, r.x + r.w / 2, r.y + (r.h - 8) / 2, THEME_FONT_SM);
    }
    display_obj.tft.setTextColor(THEME_TEXT_MUTED, THEME_BG);
    display_obj.tft.drawCentreString("Tap a level to apply", TFT_WIDTH / 2, TFT_HEIGHT - THEME_SPACE_LG, THEME_FONT_SM);
  };

  render(-1);
  led_feedback_set_brightness(levels[idx]);
  SIM_FRAME("led_brightness");

  TapDetector tap;
  int lastPressed = -1;
  while (true) {
    TouchZone zones[9];
    zones[0] = {kBackRect, ZONE_BACK};
    for (uint8_t i = 0; i < numLevels; i++) zones[i + 1] = {cellRect(i), i};

    int fired = tap.poll(zones, numLevels + 1);
    int pressedNow = -1;
    for (uint8_t i = 0; i < numLevels; i++) if (tap.isPressed(i)) pressedNow = i;
    bool backPressed = tap.isPressed(ZONE_BACK);

    if (backPressed != (lastPressed == ZONE_BACK)) drawBackButton(backPressed);
    if (pressedNow != lastPressed && pressedNow != -1) { render(pressedNow); lastPressed = pressedNow; }
    else if (pressedNow == -1 && lastPressed != -1 && lastPressed != ZONE_BACK) { lastPressed = -1; }
    if (backPressed) lastPressed = ZONE_BACK;

    if (fired == ZONE_BACK) break;
    if (fired >= 0 && fired < numLevels) {
      idx = fired;
      led_feedback_set_brightness(levels[idx]);
      render(-1);
    }

    if (encoder_turned_up() && idx > 0) { idx--; led_feedback_set_brightness(levels[idx]); render(-1); }
    if (encoder_turned_down() && idx < numLevels - 1) { idx++; led_feedback_set_brightness(levels[idx]); render(-1); }
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
