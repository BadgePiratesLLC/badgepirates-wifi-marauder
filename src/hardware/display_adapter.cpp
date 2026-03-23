#include "display_adapter.h"

#ifdef HAS_SCREEN

#include <Preferences.h>
#include "bsideskc_pins.h"

#define BL_FREQ       5000
#define BL_RESOLUTION 8

static const uint8_t BL_LEVELS[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255};
static const uint8_t BL_NUM_LEVELS = 10;
static Preferences _blPrefs;

DisplayAdapter badge_display;

void DisplayAdapter::begin() {
  pinMode(TFT_BL_PIN, OUTPUT);

  // Setup PWM on backlight pin (ESP-IDF 5.x / Arduino 3.x API)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(TFT_BL_PIN, BL_FREQ, BL_RESOLUTION);
  #else
    ledcSetup(0, BL_FREQ, BL_RESOLUTION);
    ledcAttachPin(TFT_BL_PIN, 0);
  #endif

  _blPrefs.begin("backlight", false);
  _blLevel = _blPrefs.getUChar("level", 9);
  if (_blLevel >= BL_NUM_LEVELS) _blLevel = 9;

  backlightOff();  // start dark, caller turns on after splash
}

void DisplayAdapter::backlightOn() {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(TFT_BL_PIN, BL_LEVELS[_blLevel]);
  #else
    ledcWrite(0, BL_LEVELS[_blLevel]);
  #endif
}

void DisplayAdapter::backlightOff() {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(TFT_BL_PIN, 0);
  #else
    ledcWrite(0, 0);
  #endif
}

void DisplayAdapter::backlightSet(uint8_t level) {
  if (level >= BL_NUM_LEVELS) level = BL_NUM_LEVELS - 1;
  _blLevel = level;
  backlightOn();
}

uint8_t DisplayAdapter::backlightLevel() const {
  return _blLevel;
}

void DisplayAdapter::backlightCycle() {
  _blLevel = (_blLevel + 1) % BL_NUM_LEVELS;
  backlightOn();
  backlightSave();
}

void DisplayAdapter::backlightSave() {
  _blPrefs.putUChar("level", _blLevel);
}

// ---- UI Polish Helpers (Phase 7, Issue #60) ----

#include "Display.h"
extern Display display_obj;

void DisplayAdapter::drawCenteredTitle(const char* text) {
  display_obj.tft.setTextColor(UI_COLOR_TITLE, UI_COLOR_BG);
  display_obj.tft.drawCentreString(text, TFT_WIDTH / 2, UI_TITLE_Y, UI_TITLE_FONT);
}

void DisplayAdapter::drawStatusHint(const char* text) {
  display_obj.tft.setTextColor(UI_COLOR_DIM, UI_COLOR_BG);
  display_obj.tft.drawCentreString(text, TFT_WIDTH / 2, TFT_HEIGHT - UI_STATUS_H, UI_STATUS_FONT);
}

void DisplayAdapter::drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                     uint8_t pct, uint16_t color) {
  if (pct > 100) pct = 100;
  display_obj.tft.drawRect(x, y, w, h, UI_COLOR_BODY);
  uint16_t fillW = (uint16_t)((uint32_t)(w - 4) * pct / 100);
  display_obj.tft.fillRect(x + 2, y + 2, fillW, h - 4, color);
  // Clear remainder
  if (fillW < w - 4)
    display_obj.tft.fillRect(x + 2 + fillW, y + 2, w - 4 - fillW, h - 4, UI_COLOR_BG);
}

#endif
