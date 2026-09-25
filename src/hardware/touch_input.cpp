// BSidesKC Badge - Touch input abstraction (Nexus 78e62be0)
#include "configs.h"
#ifdef HAS_SCREEN

#include "touch_input.h"
#include "Display.h"
#include "badge_ui_theme.h"

extern Display display_obj;

#ifdef BADGE_HW_CC13
#include "hardware/touch_adapter.h"

// CC13 bypasses Display::updateTouch() and reads the FT6336U directly.
//
// Display::updateTouch()'s rotation-3 branch (used for this board's 180-deg
// mount) maps a raw touch into [1,TFT_WIDTH]x[1,TFT_HEIGHT] - 240x320, this
// panel's native PRE-rotation resolution - instead of the swapped
// [.,TFT_HEIGHT]x[.,TFT_WIDTH] range rotation-1 (CC14, same landscape
// orientation family) correctly uses. See badge_ui_theme.h's
// THEME_SCREEN_W/H comment for the full story: because the render path
// pushes canvas coordinates straight into real rows/cols unscaled, that
// scale mismatch shrinks the touchable area of anything drawn near the
// bottom of a control (Nexus 78e62be0 round 2 - "back doesn't work" was
// the persistent Back button losing the bottom ~25% of its 44px target
// to this). Rather than patch upstream's Display.cpp, read the raw
// digitizer here and map straight into THEME_SCREEN_W x THEME_SCREEN_H -
// the same space badge_menu.cpp lays screens out in.
bool touchRead(uint16_t* x, uint16_t* y) {
  FT6336U& ft = getTouchController();
  if (ft.read_td_status() == 0) return false;
  uint16_t rx = ft.read_touch1_x();  // 0-239, native portrait X
  uint16_t ry = ft.read_touch1_y();  // 0-319, native portrait Y
  // Same axis reversal Kevin verified on hardware for the 180-deg mount
  // (Nexus 2977d950 / 6ed979c) - just with output bounds that match what
  // actually gets drawn, instead of the pre-rotation TFT_WIDTH/TFT_HEIGHT.
  *x = (uint16_t)map(ry, 0, 319, 0, THEME_SCREEN_W - 1);
  *y = (uint16_t)map(rx, 0, 239, THEME_SCREEN_H - 1, 0);
  return true;
}
#else
bool touchRead(uint16_t* x, uint16_t* y) {
  return display_obj.updateTouch(x, y) > 0;
}
#endif

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
