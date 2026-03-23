#pragma once
// BSidesKC Badge - Rotary Encoder Handler
// Wraps ESP32RotaryEncoder for menu navigation
// Pins: A(45), B(48), Button(20) — from bsideskc_pins.h

#include <Arduino.h>

void encoder_init();
bool encoder_turned_up();    // CW rotation consumed on read
bool encoder_turned_down();  // CCW rotation consumed on read
bool encoder_button_pressed(); // press consumed on read
