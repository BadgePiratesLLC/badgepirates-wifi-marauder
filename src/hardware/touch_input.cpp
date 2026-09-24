// BSidesKC Badge - Touch input abstraction (Nexus 78e62be0)
#include "configs.h"
#ifdef HAS_SCREEN

#include "touch_input.h"
#include "Display.h"

extern Display display_obj;

bool touchRead(uint16_t* x, uint16_t* y) {
  return display_obj.updateTouch(x, y) > 0;
}

int TapDetector::poll(const TouchZone* zones, size_t count) {
  uint16_t x = 0, y = 0;
  bool touched = touchRead(&x, &y);

  if (touched && !_down) {
    // Fresh touch-down: remember which zone (if any) it started in.
    _down = true;
    _downId = -1;
    for (size_t i = 0; i < count; i++) {
      if (zones[i].rect.contains(x, y)) {
        _downId = zones[i].id;
        break;
      }
    }
    return -1;
  }

  if (!touched && _down) {
    // Touch-up. _downId is only non-(-1) here if the finger never left
    // the zone it went down on (a drag-out cancels it below, while the
    // finger is still touching) - so firing _downId as-is already
    // implements "touch-up outside the down control cancels".
    _down = false;
    int firedId = _downId;
    _downId = -1;
    return firedId;
  }

  if (touched && _down && _downId != -1) {
    // While held, if the finger drags outside the zone that captured
    // the touch-down, cancel the gesture (spec: drag off a button
    // cancels, doesn't fire).
    bool stillInside = false;
    for (size_t i = 0; i < count; i++) {
      if (zones[i].id == _downId && zones[i].rect.contains(x, y)) {
        stillInside = true;
        break;
      }
    }
    if (!stillInside) {
      _downId = -1;  // cancelled; touch-up will now report -1
    }
  }

  return -1;
}

#endif
