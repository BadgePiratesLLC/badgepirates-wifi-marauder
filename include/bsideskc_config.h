#pragma once
// BSidesKC Badge Configuration

#include "bsideskc_pins.h"

// Board identity
#define BOARD_NAME "BSidesKC Badge"
#define BOARD_VERSION "2025"

// Our own firmware build version (Nexus 176cc276 splash screen). Hand-authored
// here, unlike MARAUDER_VERSION (extracted from upstream's real configs.h at
// build time into BP_MARAUDER_VERSION_UPSTREAM, see include/marauder_config.h
// + scripts/inject_build_info.py) and the git SHA (also injected at build
// time, see platformio.ini/sim/build.sh) - this one IS the literal, bump it
// by hand when we cut a build.
#define BP_BUILD_VERSION "1.0.0"

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
