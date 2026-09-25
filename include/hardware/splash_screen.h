#pragma once
// Boot splash (Nexus 176cc276): Badge Pirates skull, wordmark, "Marauder
// build" line, and a version block that's derived at build time, never
// hand-typed. Painted to LVGL's draw buffer and pushed once, then the
// caller raises the backlight - display_adapter.cpp's begin() already
// starts the panel dark for exactly this ("caller turns on after splash").

#include "configs.h"
#ifdef HAS_SCREEN

#include <lvgl.h>

// Builds the splash screen, paints it, and loads it as the active LVGL
// screen - one complete frame, no partial/progressive draw visible. Call
// this BEFORE backlightOn(). Returns the screen object; the caller owns
// it and must pass it to splashDismissWait() to free it.
lv_obj_t* splashShow();

// Blocks until the LATER of {SPLASH_MIN_MS elapsed since startMs, a
// touch}, then deletes `screen`. Call this after the rest of setup()'s
// real init work has run: that work already consumes most (or all) of
// the hold budget, so this only ever waits out whatever's left - never an
// extra delay() stacked on top of boot work. A touch skips the remainder
// immediately.
void splashDismissWait(uint32_t startMs, lv_obj_t* screen);

#endif
