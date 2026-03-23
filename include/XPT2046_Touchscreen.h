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

    // Return raw touch coordinates.
    // Upstream CYD path maps these with map() in Display::updateTouch.
    // We return values pre-scaled to match the expected 200-3700 / 240-3800
    // range so the existing map() calls in Display.cpp produce correct screen coords.
    TS_Point getPoint() {
        FT6336U& ft = getTouchController();
        uint16_t rx = ft.read_touch1_x();
        uint16_t ry = ft.read_touch1_y();

        // FT6336U returns 0-239 (X) and 0-319 (Y) in portrait.
        // Scale to the resistive-touch ADC range that Display.cpp map() expects.
        int16_t sx = map(rx, 0, 239, 200, 3700);
        int16_t sy = map(ry, 0, 319, 240, 3800);

        return TS_Point(sx, sy, 100); // z=100 indicates pressed
    }
};
