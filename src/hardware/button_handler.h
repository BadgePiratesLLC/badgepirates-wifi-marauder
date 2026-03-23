#pragma once
// BSidesKC Badge - Button Handler
// Maps badge buttons (BOOT, ENTER, BACK) to Marauder's Switches interface
// BOOT button provides extra functionality not in upstream Marauder

#include <Arduino.h>
#include "bsideskc_pins.h"

// Button indices
enum BadgeButton : uint8_t {
  BTN_IDX_BOOT  = 0,
  BTN_IDX_ENTER = 1,
  BTN_IDX_BACK  = 2,
  BTN_COUNT     = 3
};

// Event types for BOOT button (not handled by Switches)
enum ButtonEvent : uint8_t {
  BTN_EVT_NONE    = 0,
  BTN_EVT_PRESS   = 1,
  BTN_EVT_RELEASE = 2,
  BTN_EVT_HOLD    = 3
};

// Initialize all badge buttons (call in setup before Switches objects are used)
void buttonHandlerInit();

// Poll BOOT button — call each loop iteration. Returns event type.
ButtonEvent buttonBootPoll();

// Check if BOOT is currently held (for long-press actions like brightness)
bool buttonBootHeld();
