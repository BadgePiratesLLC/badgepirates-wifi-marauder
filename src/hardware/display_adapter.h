#pragma once
// BSidesKC Badge Display Adapter
// Wraps TFT_eSPI init for badge-specific ILI9341 (320x240, backlight GPIO 6)
// Works alongside upstream Display class — handles badge HW before Display::RunSetup()

#include "configs.h"

#ifdef HAS_SCREEN

#include <Arduino.h>

class DisplayAdapter {
public:
  void begin();           // Badge-specific pre-init (backlight PWM, pin modes)
  void backlightOn();
  void backlightOff();
  void backlightSet(uint8_t level);  // 0-9 index into brightness table
  uint8_t backlightLevel() const;
  void backlightCycle();
  void backlightSave();

private:
  uint8_t _blLevel = 9;  // default full brightness
};

extern DisplayAdapter badge_display;

#endif
