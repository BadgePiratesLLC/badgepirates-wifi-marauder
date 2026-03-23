#pragma once
// BSidesKC Badge Configuration

#include "bsideskc_pins.h"

// Board identity
#define BOARD_NAME "BSidesKC Badge"
#define BOARD_VERSION "2025"

// Display
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Feature flags (mirrors Marauder configs.h pattern)
#define HAS_SCREEN
#define HAS_FULL_SCREEN
#define HAS_TOUCH
#define HAS_BT
#define HAS_IDF_3
#define HAS_SD
#define USE_SD
#define HAS_NEOPIXEL_LED
#define HAS_BUTTONS
#define HAS_BATTERY
