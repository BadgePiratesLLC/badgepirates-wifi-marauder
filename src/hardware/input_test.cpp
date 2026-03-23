#include "hardware/input_test.h"
#include "configs.h"

#ifdef HAS_SCREEN
#include "Display.h"
#include "hardware/display_adapter.h"
#include "hardware/button_handler.h"
#include "hardware/encoder_handler.h"

extern Display display_obj;

// Status tracking for each input component
static struct {
  bool touch;
  bool enter;
  bool back;
  bool boot;
  bool enc_cw;
  bool enc_ccw;
  bool enc_btn;
} _validated;

static int _enc_pos = 0;
static uint8_t _bl_level = 9;

static void drawHeader() {
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.drawCentreString("INPUT VALIDATION", TFT_WIDTH / 2, 2, 2);
  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.drawCentreString("Hold BOOT+BACK to exit", TFT_WIDTH / 2, TFT_HEIGHT - 14, 1);
}

static void drawChecklist() {
  int y = 24;
  const int x = 4;
  auto row = [&](const char* label, bool ok) {
    display_obj.tft.fillRect(x, y, TFT_WIDTH / 2 - 4, 14, TFT_BLACK);
    display_obj.tft.setTextColor(ok ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
    display_obj.tft.setCursor(x, y);
    display_obj.tft.printf("[%c] %s", ok ? 'X' : ' ', label);
    y += 16;
  };
  row("Touch",      _validated.touch);
  row("ENTER btn",  _validated.enter);
  row("BACK btn",   _validated.back);
  row("BOOT btn",   _validated.boot);
  row("Encoder CW", _validated.enc_cw);
  row("Encoder CCW",_validated.enc_ccw);
  row("Encoder Btn",_validated.enc_btn);
}

static void drawStatus() {
  int rx = TFT_WIDTH / 2 + 4;
  int y = 24;
  display_obj.tft.fillRect(rx, y, TFT_WIDTH / 2 - 4, 100, TFT_BLACK);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.setCursor(rx, y);
  display_obj.tft.printf("Enc pos: %d", _enc_pos);
  y += 16;
  display_obj.tft.setCursor(rx, y);
  display_obj.tft.printf("BL level: %d", _bl_level);
  y += 16;
  display_obj.tft.setCursor(rx, y);
  display_obj.tft.printf("Touch: ---");
}

static void updateTouchCoord(uint16_t tx, uint16_t ty) {
  int rx = TFT_WIDTH / 2 + 4;
  int y = 24 + 32;
  display_obj.tft.fillRect(rx, y, TFT_WIDTH / 2 - 4, 14, TFT_BLACK);
  display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  display_obj.tft.setCursor(rx, y);
  display_obj.tft.printf("Touch: %d,%d", tx, ty);
}

void runInputValidationTest() {
  memset(&_validated, 0, sizeof(_validated));
  _enc_pos = 0;
  _bl_level = badge_display.backlightLevel();

  drawHeader();
  drawChecklist();
  drawStatus();

  Serial.println(F("[InputTest] Validation mode active"));

  while (true) {
    // Touch
    uint16_t tx, ty;
    if (display_obj.updateTouch(&tx, &ty)) {
      if (!_validated.touch) { _validated.touch = true; drawChecklist(); }
      updateTouchCoord(tx, ty);
      display_obj.tft.fillCircle(tx, ty, 2, TFT_MAGENTA);
      Serial.printf("[InputTest] Touch x=%d y=%d\n", tx, ty);
    }

    // ENTER button (active-low, handled by Switches but we read raw)
    if (digitalRead(C_BTN) == LOW) {
      if (!_validated.enter) { _validated.enter = true; drawChecklist(); }
      Serial.println(F("[InputTest] ENTER pressed"));
      while (digitalRead(C_BTN) == LOW) delay(10); // wait release
    }

    // BACK button
    if (digitalRead(D_BTN) == LOW) {
      if (!_validated.back) { _validated.back = true; drawChecklist(); }
      Serial.println(F("[InputTest] BACK pressed"));
      // Check exit combo: BOOT + BACK held together
      if (digitalRead(BTN_BOOT) == LOW) {
        Serial.println(F("[InputTest] EXIT combo detected"));
        break;
      }
      while (digitalRead(D_BTN) == LOW) delay(10);
    }

    // BOOT button
    ButtonEvent evt = buttonBootPoll();
    if (evt == BTN_EVT_PRESS || evt == BTN_EVT_RELEASE) {
      if (!_validated.boot) { _validated.boot = true; drawChecklist(); }
      Serial.println(F("[InputTest] BOOT pressed"));
    }
    if (evt == BTN_EVT_HOLD) {
      // Long-press cycles backlight as demo
      badge_display.backlightCycle();
      _bl_level = badge_display.backlightLevel();
      drawStatus();
      Serial.printf("[InputTest] Backlight -> %d\n", _bl_level);
    }

    // Encoder rotation
    if (encoder_turned_up()) {
      _enc_pos--;
      if (!_validated.enc_ccw) { _validated.enc_ccw = true; drawChecklist(); }
      int rx = TFT_WIDTH / 2 + 4;
      display_obj.tft.fillRect(rx, 24, TFT_WIDTH / 2 - 4, 14, TFT_BLACK);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.setCursor(rx, 24);
      display_obj.tft.printf("Enc pos: %d", _enc_pos);
      Serial.printf("[InputTest] Enc CCW pos=%d\n", _enc_pos);
    }
    if (encoder_turned_down()) {
      _enc_pos++;
      if (!_validated.enc_cw) { _validated.enc_cw = true; drawChecklist(); }
      int rx = TFT_WIDTH / 2 + 4;
      display_obj.tft.fillRect(rx, 24, TFT_WIDTH / 2 - 4, 14, TFT_BLACK);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.setCursor(rx, 24);
      display_obj.tft.printf("Enc pos: %d", _enc_pos);
      Serial.printf("[InputTest] Enc CW pos=%d\n", _enc_pos);
    }

    // Encoder button
    if (encoder_button_pressed()) {
      if (!_validated.enc_btn) { _validated.enc_btn = true; drawChecklist(); }
      Serial.println(F("[InputTest] Enc button pressed"));
    }

    delay(20);
  }

  // Summary
  int passed = _validated.touch + _validated.enter + _validated.back +
               _validated.boot + _validated.enc_cw + _validated.enc_ccw +
               _validated.enc_btn;
  Serial.printf("[InputTest] Result: %d/7 inputs validated\n", passed);
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(passed == 7 ? TFT_GREEN : TFT_YELLOW, TFT_BLACK);
  display_obj.tft.drawCentreString(
    passed == 7 ? "ALL PASS" : "INCOMPLETE",
    TFT_WIDTH / 2, TFT_HEIGHT / 2 - 10, 2);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.drawCentreString(
    String(passed) + "/7 validated",
    TFT_WIDTH / 2, TFT_HEIGHT / 2 + 14, 1);
  delay(2000);
}

#endif
