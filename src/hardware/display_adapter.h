#pragma once
// BSidesKC Badge Display Adapter
// Wraps TFT_eSPI init for badge-specific ILI9341 (320x240, backlight GPIO 6)
// Works alongside upstream Display class — handles badge HW before Display::RunSetup()

#include "configs.h"

#ifdef HAS_SCREEN

#include <Arduino.h>

// UI polish constants for 320×240 display
#define UI_MARGIN         4
#define UI_TITLE_Y        4
#define UI_TITLE_FONT     2    // Font 2 = 16px, readable on 240px width
#define UI_BODY_FONT      1    // Font 1 = 8px, for dense info
#define UI_STATUS_FONT    1
#define UI_STATUS_H       16
#define UI_BAR_H          24
#define UI_BAR_MARGIN     30
#define UI_COLOR_TITLE    0x07FF  // TFT_CYAN
#define UI_COLOR_BODY     0xFFFF  // TFT_WHITE
#define UI_COLOR_DIM      0x7BEF  // TFT_DARKGREY
#define UI_COLOR_OK       0x07E0  // TFT_GREEN
#define UI_COLOR_WARN     0xFFE0  // TFT_YELLOW
#define UI_COLOR_ERR      0xF800  // TFT_RED
#define UI_COLOR_BG       0x0000  // TFT_BLACK

class DisplayAdapter {
public:
  void begin();           // Badge-specific pre-init (backlight PWM, pin modes)
  void backlightOn();
  void backlightOff();
  void backlightSet(uint8_t level);  // 0-9 index into brightness table
  uint8_t backlightLevel() const;
  void backlightCycle();
  void backlightSave();

  // UI polish helpers for 320×240
  void drawCenteredTitle(const char* text);
  void drawStatusHint(const char* text);
  void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint8_t pct, uint16_t color);

private:
  uint8_t _blLevel = 9;  // default full brightness
};

extern DisplayAdapter badge_display;

#endif
