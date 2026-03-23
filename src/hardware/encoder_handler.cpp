#include "encoder_handler.h"
#include <ESP32RotaryEncoder.h>
#include "bsideskc_pins.h"

static RotaryEncoder encoder(ENC_A_PIN, ENC_B_PIN, ENC_BTN_PIN);

static volatile bool _turnedUp = false;
static volatile bool _turnedDown = false;
static volatile bool _buttonPressed = false;

void encoder_init() {
  encoder.setEncoderType(EncoderType::FLOATING);
  encoder.setBoundaries(-1, 1, false);
  encoder.onTurned([](long value) {
    if (value > 0) _turnedDown = true;   // CW = scroll down
    else if (value < 0) _turnedUp = true; // CCW = scroll up
    encoder.setEncoderValue(0);
  });
  encoder.onPressed([](unsigned long) {
    _buttonPressed = true;
  });
  encoder.begin();
}

bool encoder_turned_up() {
  bool f = _turnedUp;
  _turnedUp = false;
  return f;
}

bool encoder_turned_down() {
  bool f = _turnedDown;
  _turnedDown = false;
  return f;
}

bool encoder_button_pressed() {
  bool f = _buttonPressed;
  _buttonPressed = false;
  return f;
}
