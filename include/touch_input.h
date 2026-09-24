#pragma once
// BSidesKC Badge - Touch input abstraction (Nexus 78e62be0)
//
// Wraps upstream Display::updateTouch() (already pixel-accurate for this
// panel, see XPT2046_Touchscreen.h shim) with named on-screen rects and a
// touch-down/touch-up debounce rule: a control only "fires" if the
// touch-up lands inside the SAME rect that received the touch-down. A
// drag off the button cancels instead of firing. This is the one true
// input primitive new touch screens should poll instead of hand-rolling
// their own updateTouch() loop.

#include <Arduino.h>

#ifdef HAS_SCREEN

// Axis-aligned tap target. IDs are caller-defined small ints (screen-local).
struct UiRect {
  int16_t x, y, w, h;
  bool contains(uint16_t px, uint16_t py) const {
    return px >= x && px < (x + w) && py >= y && py < (y + h);
  }
};

struct TouchZone {
  UiRect rect;
  uint8_t id;
};

// Tracks one touch gesture across frames. Call poll() once per loop
// iteration with the current frame's zone list.
class TapDetector {
public:
  // Returns the id of a zone whose touch-down AND touch-up both landed
  // inside it (a completed tap), or -1 if nothing fired this frame.
  // Also reports live press state via isPressed(id) so callers can draw
  // an immediate pressed/highlight visual on touch-down.
  int poll(const TouchZone* zones, size_t count);

  // True while a zone is currently held down (for pressed-state drawing).
  bool isPressed(uint8_t id) const { return _down && _downId == id; }

private:
  bool _down = false;
  int _downId = -1;
};

// Raw touch read, pixel coordinates. Thin wrapper so callers don't need
// to know about display_obj directly.
bool touchRead(uint16_t* x, uint16_t* y);

#endif
