#pragma once
// BSidesKC Badge simulator — fake led_feedback (real header pulls in
// Adafruit_NeoPixel, which doesn't exist on the host; the NeoPixel ring
// isn't part of what this ticket needs to prove, so it's stubbed, not
// modeled).

#include "Arduino.h"

enum LedState : uint8_t {
    LED_IDLE,
    LED_SCANNING,
    LED_ATTACK,
    LED_CAPTURE_OK,
    LED_ERROR
};

void led_feedback_init();
void led_feedback_set(LedState state);
void led_feedback_start_task();
void led_feedback_set_brightness(uint8_t brightness);
uint8_t led_feedback_get_brightness();
