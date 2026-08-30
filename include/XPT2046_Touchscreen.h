#pragma once
// BSidesKC Badge - XPT2046_Touchscreen Shim
// Drop-in replacement: provides XPT2046_Touchscreen API backed by FT6336U.
// Placed in include/ so it shadows the real XPT2046 library header.

#include <Arduino.h>
#include "hardware/touch_adapter.h"

// Matches upstream TS_Point used by Display.cpp
struct TS_Point {
    int16_t x, y, z;
    TS_Point() : x(0), y(0), z(0) {}
    TS_Point(int16_t _x, int16_t _y, int16_t _z) : x(_x), y(_y), z(_z) {}
};

// Mimics XPT2046_Touchscreen interface consumed by Display.h / Display.cpp
class XPT2046_Touchscreen {
public:
    // Constructor signature matches upstream (cs, irq)
    XPT2046_Touchscreen(uint8_t, uint8_t) {}

    // begin() accepts SPIClass& in upstream; we ignore it and init FT6336U
    void begin(SPIClass&) { touchAdapterInit(); }

    void setRotation(uint8_t) {} // FT6336U doesn't need SPI rotation

    // Returns true when touch IRQ is asserted (touch active)
    bool tirqTouched() {
        return getTouchController().read_td_status() > 0;
    }

    // Returns true when a valid touch point exists
    bool touched() {
        return getTouchController().read_td_status() > 0;
    }

    // Return touch coordinates scaled to fake XPT2046 ADC range.
    // Display::updateTouch() reads tft.getRotation() and applies the
    // appropriate map() to convert these back to screen pixels.
    //
    // Upstream case 1 (landscape, CC14/BSidesKC26 — SCREEN_ORIENTATION 1) does:
    //   screen_x = map(p.y, 143, 3715, 0, TFT_HEIGHT)
    //   screen_y = map(p.x, 3786, 216, 0, TFT_WIDTH)
    // So p.y must span 143-3715 and p.x must span 216-3786.
    //
    // Upstream case 3 (landscape 180° flipped, CC13/BSidesKC25 —
    // SCREEN_ORIENTATION 3, Nexus 2977d950) does:
    //   screen_x = map(p.y, 3800, 240, 1, TFT_WIDTH)
    //   screen_y = map(p.x, 200, 3700, 1, TFT_HEIGHT)
    // Mirrored vs case 1 (endpoints swapped) to match the panel being
    // mounted rotated 180° on this hardware revision. Needs verification
    // against physical CC13 hardware — touch orientation may need
    // re-deriving if the digitizer itself isn't wired the same way as the
    // display.
    TS_Point getPoint() {
        FT6336U& ft = getTouchController();
        uint16_t rx = ft.read_touch1_x();
        uint16_t ry = ft.read_touch1_y();

        // FT6336U returns 0-239 (X) and 0-319 (Y) in native portrait.
#ifdef BADGE_HW_CC13
        // Map to the exact ADC ranges upstream case 3 expects (mirrored
        // vs CC14's case 1 ranges below).
        int16_t sx = map(rx, 0, 239, 200, 3700);
        int16_t sy = map(ry, 0, 319, 240, 3800);
#else
        // Map to the exact ADC ranges upstream case 1 expects.
        int16_t sx = map(rx, 0, 239, 216, 3786);
        int16_t sy = map(ry, 0, 319, 143, 3715);
#endif

        return TS_Point(sx, sy, 100); // z=100 indicates pressed
    }
};
