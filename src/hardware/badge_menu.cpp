// BSidesKC Badge - Badge-specific menu integration
// Nexus 78e62be0: touch-first UI overhaul (half 1 of 2).
//
// Root/idle screen, the "Badge" submenu, and the LED Brightness options
// screen are now fully custom-drawn on our side (theme + touch + a small
// generic back-stack) instead of leaning on upstream's encoder-driven
// Menu/MenuNode rendering. Everything else (the ~20 remaining upstream
// screens, including Marauder's own main menu once you tap into it) is
// untouched this half - see Nexus ticket for the scope boundary.

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
#include "badge_ui_theme.h"
#include "touch_input.h"
#include "badge_nav.h"

extern MenuFunctions menu_function_obj;
extern Display display_obj;

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

static void badgeIdleScreen();
static void drawBadgeSubmenu();
static void ledBrightnessOptionsScreen();
static void showBatteryStatus();
static void toggleBuzzerMute();
static void runHwTest();

// ---------------------------------------------------------------------
// Root / idle screen (Nexus req #12: identity lives here; no Back).
// ---------------------------------------------------------------------

static void badgeIdleScreen() {
  badgeNavPush(badgeIdleScreen, "");  // root marker; badgeNavIsRoot() == true here

  display_obj.tft.fillScreen(THEME_BG);
  drawChrome("", /*isRoot=*/true);

  display_obj.tft.setTextColor(THEME_ACCENT, THEME_BG);
  display_obj.tft.drawCentreString("BADGE PIRATES", TFT_WIDTH / 2, 60, THEME_FONT_LG);
  display_obj.tft.setTextColor(THEME_TEXT_MUTED, THEME_BG);
  display_obj.tft.drawCentreString("CC13", TFT_WIDTH / 2, 90, THEME_FONT_MD);

  const uint8_t ZONE_MENU = 1;
  CardButton menuBtn{{THEME_SPACE_LG, 140, TFT_WIDTH - THEME_SPACE_LG * 2, THEME_CARD_MIN_H + 10}, "Menu", "Tap to open", false};
  drawCard(menuBtn, false);
  display_obj.tft.setTextColor(THEME_TEXT_MUTED, THEME_BG);
  display_obj.tft.drawCentreString("Tap Menu to begin", TFT_WIDTH / 2, TFT_HEIGHT - 20, THEME_FONT_SM);

  TapDetector tap;
  bool lastPressed = false;
  while (true) {
    TouchZone zones[] = {{menuBtn.rect, ZONE_MENU}};
    int fired = tap.poll(zones, 1);
    bool pressedNow = tap.isPressed(ZONE_MENU);
    if (pressedNow != lastPressed) {
      drawCard(menuBtn, pressedNow);
      lastPressed = pressedNow;
    }
    if (fired == ZONE_MENU || encoder_button_pressed()) break;
    delay(30);
  }

  menu_function_obj.changeMenu(s_mainMenu, true);
  badgeNavPop();
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

static void ledBrightnessOptionsScreen() {
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
// off to upstream's Menu/MenuNode renderer. Finish by showing our new
// root/idle screen instead of dropping straight into upstream's main menu.
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

  badgeIdleScreen();
}

#endif
