#pragma once
// BSidesKC Badge - Buzzer Interface
// PWM tone output on GPIO 19 for audio feedback

#include <Arduino.h>
#include "bsideskc_pins.h"

// Tone pattern identifiers
enum BuzzerTone : uint8_t {
  TONE_BUTTON_PRESS = 0,
  TONE_SCAN_START,
  TONE_SCAN_STOP,
  TONE_CAPTURE_SUCCESS,
  TONE_ERROR,
  TONE_LOW_BATTERY
};

void buzzerInit();
void buzzerPlay(BuzzerTone tone);
void buzzerUpdate();  // call from loop() to handle async patterns
void buzzerMute(bool muted);
bool buzzerIsMuted();
