#include "UI/StatusBar.h"
#include "UI/CardKit.h"
#include "badge_ui_theme.h"
#include "hardware/battery_monitor.h"
#include "hardware/radio_status.h"
#include "hardware/ntp_clock.h"
#include <cstdio>
#include <cstring>

// Battery is the one genuinely slow read here (I2C fuel gauge, not a
// GPIO/state flag) - "the battery can poll slowly (30s+)" per the ticket.
static const uint32_t BATTERY_POLL_MS = 30000;

static lv_obj_t* s_bar = nullptr;
static lv_obj_t* s_backBtn = nullptr;
static lv_obj_t* s_backLabel = nullptr;
static lv_obj_t* s_gearBtn = nullptr;
static lv_obj_t* s_battShell = nullptr;
static lv_obj_t* s_battFill = nullptr;
static lv_obj_t* s_battLabel = nullptr;
static lv_obj_t* s_wifiIcon = nullptr;
static lv_obj_t* s_btIcon = nullptr;
static lv_obj_t* s_clockLabel = nullptr;

// Last-drawn state, diffed so statusbar_pollIndicators() (called every
// tick) only touches LVGL objects when a value actually changed.
static bool s_shownBack = false;
static bool s_backWasPressed = false;
static bool s_gearWasPressed = false;
static int s_lastBattPct = -2;            // -2: never polled, forces the first paint
static uint32_t s_lastBattPollMs = 0;
static bool s_battEverPolled = false;
static bool s_lastWifi = false;
static bool s_lastWifiKnown = false;
static bool s_lastBt = false;
static bool s_lastBtKnown = false;
static bool s_lastClockShown = false;
static char s_lastClockStr[6] = {0};

static void style_navbtn(lv_obj_t* btn, bool pressed) {
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(btn, cardkit_color(pressed ? THEME_SURFACE_HI : THEME_BG), 0);
  lv_obj_set_style_radius(btn, 0, 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_pad_all(btn, 0, 0);
}

void statusbar_create(lv_obj_t* parent) {
  s_bar = lv_obj_create(parent);
  lv_obj_remove_flag(s_bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(s_bar, 0, 0);
  lv_obj_set_size(s_bar, TFT_WIDTH, THEME_STATUSBAR_H);
  lv_obj_set_style_radius(s_bar, 0, 0);
  lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_bar, cardkit_color(THEME_BG), 0);
  lv_obj_set_style_pad_all(s_bar, 0, 0);
  lv_obj_set_style_border_side(s_bar, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(s_bar, 1, 0);
  lv_obj_set_style_border_color(s_bar, cardkit_color(THEME_BORDER), 0);

  // Back: icon-only (not "< Back" text - that's what freed the width the
  // status cluster needed), left edge, hidden on root by statusbar_setChrome().
  s_backBtn = lv_obj_create(s_bar);
  lv_obj_remove_flag(s_backBtn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(s_backBtn, 0, 0);
  lv_obj_set_size(s_backBtn, THEME_BACK_W, THEME_BACK_H);
  style_navbtn(s_backBtn, false);
  lv_obj_add_flag(s_backBtn, LV_OBJ_FLAG_HIDDEN);
  s_backLabel = lv_label_create(s_backBtn);
  lv_label_set_text(s_backLabel, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(s_backLabel, cardkit_color(THEME_TEXT), 0);
  lv_obj_set_style_text_font(s_backLabel, &lv_font_montserrat_14, 0);
  lv_obj_center(s_backLabel);

  // Gear: always present, right edge. Only tappable control besides
  // Back - "tapping the battery or WiFi glyph must do nothing."
  s_gearBtn = lv_obj_create(s_bar);
  lv_obj_remove_flag(s_gearBtn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(s_gearBtn, TFT_WIDTH - THEME_GEAR_W, 0);
  lv_obj_set_size(s_gearBtn, THEME_GEAR_W, THEME_STATUSBAR_H);
  style_navbtn(s_gearBtn, false);
  lv_obj_t* gearLabel = lv_label_create(s_gearBtn);
  lv_label_set_text(gearLabel, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_color(gearLabel, cardkit_color(THEME_TEXT), 0);
  lv_obj_set_style_text_font(gearLabel, &lv_font_montserrat_14, 0);
  lv_obj_center(gearLabel);

  // Status cluster: battery | WiFi | BT | clock, right-packed in the
  // space between Back and the gear. A flex row (not hand-placed x
  // coordinates) so the clock can disappear/reappear (Nexus c39cd3b3:
  // "show the clock only when actually synced, hide it otherwise")
  // without leaving a dead gap or needing its neighbours repositioned.
  lv_obj_t* cluster = lv_obj_create(s_bar);
  lv_obj_remove_flag(cluster, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(cluster, THEME_BACK_W, 0);
  lv_obj_set_size(cluster, TFT_WIDTH - THEME_BACK_W - THEME_GEAR_W, THEME_STATUSBAR_H);
  lv_obj_set_style_bg_opa(cluster, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cluster, 0, 0);
  lv_obj_set_style_pad_all(cluster, 0, 0);
  lv_obj_set_style_pad_right(cluster, 6, 0);
  lv_obj_set_style_pad_column(cluster, 6, 0);
  lv_obj_set_flex_flow(cluster, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cluster, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  s_battShell = lv_obj_create(cluster);
  lv_obj_remove_flag(s_battShell, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(s_battShell, THEME_BATT_W, THEME_BATT_H);
  lv_obj_set_style_radius(s_battShell, 2, 0);
  lv_obj_set_style_bg_opa(s_battShell, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(s_battShell, 1, 0);
  lv_obj_set_style_border_color(s_battShell, cardkit_color(THEME_TEXT_MUTED), 0);
  lv_obj_set_style_pad_all(s_battShell, 2, 0);
  s_battFill = lv_obj_create(s_battShell);
  lv_obj_remove_flag(s_battFill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(s_battFill, 0, 0);
  lv_obj_set_style_border_width(s_battFill, 0, 0);
  lv_obj_set_style_radius(s_battFill, 1, 0);
  lv_obj_set_style_bg_opa(s_battFill, LV_OPA_COVER, 0);
  lv_obj_set_pos(s_battFill, 0, 0);
  lv_obj_set_size(s_battFill, 1, THEME_BATT_H - 4);

  s_battLabel = lv_label_create(cluster);
  lv_obj_set_style_text_font(s_battLabel, &lv_font_montserrat_12, 0);
  lv_label_set_text(s_battLabel, "");

  s_wifiIcon = lv_label_create(cluster);
  lv_label_set_text(s_wifiIcon, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(s_wifiIcon, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_wifiIcon, cardkit_color(THEME_TEXT_MUTED), 0);

  s_btIcon = lv_label_create(cluster);
  lv_label_set_text(s_btIcon, LV_SYMBOL_BLUETOOTH);
  lv_obj_set_style_text_font(s_btIcon, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_btIcon, cardkit_color(THEME_TEXT_MUTED), 0);

  s_clockLabel = lv_label_create(cluster);
  lv_obj_set_style_text_font(s_clockLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(s_clockLabel, cardkit_color(THEME_TEXT), 0);
  lv_label_set_text(s_clockLabel, "");
  lv_obj_add_flag(s_clockLabel, LV_OBJ_FLAG_HIDDEN);  // conditional: never show 00:00 or a stale time
}

void statusbar_setChrome(bool showBack, bool backPressed, bool gearPressed) {
  if (showBack != s_shownBack) {
    s_shownBack = showBack;
    if (showBack) lv_obj_remove_flag(s_backBtn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_backBtn, LV_OBJ_FLAG_HIDDEN);
  }
  if (showBack && backPressed != s_backWasPressed) {
    s_backWasPressed = backPressed;
    style_navbtn(s_backBtn, backPressed);
  }
  if (gearPressed != s_gearWasPressed) {
    s_gearWasPressed = gearPressed;
    style_navbtn(s_gearBtn, gearPressed);
  }
}

static void pollBattery(uint32_t currentTime) {
  if (s_battEverPolled && (currentTime - s_lastBattPollMs) < BATTERY_POLL_MS) return;
  s_battEverPolled = true;
  s_lastBattPollMs = currentTime;

  int pct = batteryGetPercent();
  if (pct == s_lastBattPct) return;
  s_lastBattPct = pct;

  if (pct < 0) {
    // No sense path detected (BatteryInterface::getBatteryLevel()'s
    // "no chip responded on I2C" case) - charge-state glyph only, no
    // number. A hardcoded/guessed percent would be worse than none.
    lv_label_set_text(s_battLabel, "");
    lv_obj_set_size(s_battFill, 1, THEME_BATT_H - 4);
    lv_obj_set_style_bg_opa(s_battFill, LV_OPA_TRANSP, 0);
    return;
  }

  int clamped = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  char buf[6];
  snprintf(buf, sizeof(buf), "%d%%", clamped);
  lv_label_set_text(s_battLabel, buf);

  uint16_t battColor = (clamped > 50) ? THEME_OK : (clamped > 20) ? THEME_WARN : THEME_ERROR;
  int fillW = (THEME_BATT_W - 4) * clamped / 100;
  lv_obj_set_style_bg_opa(s_battFill, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_battFill, cardkit_color(battColor), 0);
  lv_obj_set_size(s_battFill, fillW < 1 ? 1 : fillW, THEME_BATT_H - 4);
}

// ON = full-brightness text color, OFF = the muted token - a full colour
// swap, not an opacity fade. "Dimmed/off state must be visibly distinct
// at arm's length, not a subtle grey shift" (Nexus c39cd3b3) - an alpha
// fade toward a near-black background reads as "broken," not "off."
static void setIconOn(lv_obj_t* icon, bool on) {
  lv_obj_set_style_text_color(icon, cardkit_color(on ? THEME_TEXT : THEME_TEXT_MUTED), 0);
}

void statusbar_pollIndicators(uint32_t currentTime) {
  pollBattery(currentTime);

  bool wifi = wifiIsUp();
  if (!s_lastWifiKnown || wifi != s_lastWifi) {
    s_lastWifiKnown = true;
    s_lastWifi = wifi;
    setIconOn(s_wifiIcon, wifi);
  }

  bool bt = bluetoothIsUp();
  if (!s_lastBtKnown || bt != s_lastBt) {
    s_lastBtKnown = true;
    s_lastBt = bt;
    setIconOn(s_btIcon, bt);
  }

  ntpClockUpdate(wifi, currentTime);
  bool synced = ntpIsSynced();
  if (!synced) {
    if (s_lastClockShown) {
      s_lastClockShown = false;
      s_lastClockStr[0] = '\0';
      lv_obj_add_flag(s_clockLabel, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  char buf[6];
  ntpGetTimeString(buf, sizeof(buf));
  if (!s_lastClockShown || strcmp(buf, s_lastClockStr) != 0) {
    s_lastClockShown = true;
    snprintf(s_lastClockStr, sizeof(s_lastClockStr), "%s", buf);
    lv_label_set_text(s_clockLabel, buf);
    lv_obj_remove_flag(s_clockLabel, LV_OBJ_FLAG_HIDDEN);
  }
}
