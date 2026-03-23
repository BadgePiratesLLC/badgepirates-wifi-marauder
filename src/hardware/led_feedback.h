#pragma once
#include <Adafruit_NeoPixel.h>

// Badge LED states — driven by Marauder scan modes
enum LedState : uint8_t {
    LED_IDLE,           // Breathing cyan
    LED_SCANNING,       // Rotating blue
    LED_ATTACK,         // Pulsing red
    LED_CAPTURE_OK,     // Green flash (auto-returns to previous)
    LED_ERROR           // Red blink (auto-returns to previous)
};

void led_feedback_init();
void led_feedback_set(LedState state);
void led_feedback_update();  // Call from loop()
void led_feedback_set_brightness(uint8_t brightness);
uint8_t led_feedback_get_brightness();
