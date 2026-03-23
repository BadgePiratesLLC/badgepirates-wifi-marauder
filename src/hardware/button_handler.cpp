#include "hardware/button_handler.h"

// BOOT button state (ENTER/BACK are handled by Marauder's Switches c_btn/d_btn)
static bool     boot_pressed    = false;
static bool     boot_held       = false;
static bool     boot_was_held   = false;  // latched until release
static uint32_t boot_press_time = 0;
static const uint32_t DEBOUNCE_MS = 50;
static const uint32_t HOLD_MS    = 1000;
static uint32_t boot_last_read  = 0;

void buttonHandlerInit() {
  pinMode(BTN_BOOT,  INPUT_PULLUP);
  // ENTER and BACK are initialized by Switches constructor in main.cpp
}

ButtonEvent buttonBootPoll() {
  uint32_t now = millis();
  if (now - boot_last_read < DEBOUNCE_MS) return BTN_EVT_NONE;
  boot_last_read = now;

  bool state = (digitalRead(BTN_BOOT) == LOW);  // active-low

  if (state && !boot_pressed) {
    boot_pressed = true;
    boot_held = false;
    boot_was_held = false;
    boot_press_time = now;
    return BTN_EVT_PRESS;
  }

  if (state && boot_pressed) {
    if (!boot_held && (now - boot_press_time >= HOLD_MS)) {
      boot_held = true;
      boot_was_held = true;
      return BTN_EVT_HOLD;
    }
    return BTN_EVT_NONE;
  }

  if (!state && boot_pressed) {
    boot_pressed = false;
    bool was_held = boot_was_held;
    boot_held = false;
    boot_was_held = false;
    return was_held ? BTN_EVT_NONE : BTN_EVT_RELEASE;
  }

  return BTN_EVT_NONE;
}

bool buttonBootHeld() {
  return boot_held;
}
