// BSidesKC Badge - Screen back-stack (Nexus 78e62be0)
#include "configs.h"
#ifdef HAS_SCREEN

#include "badge_nav.h"

#define NAV_MAX_DEPTH 8

static struct {
  BadgeScreenFn fn;
  const char* title;
} _stack[NAV_MAX_DEPTH];

static uint8_t _depth = 0;  // number of entries below (0 == root screen)

void badgeNavPush(BadgeScreenFn fn, const char* title) {
  if (_depth < NAV_MAX_DEPTH) {
    _stack[_depth].fn = fn;
    _stack[_depth].title = title;
    _depth++;
  }
}

void badgeNavPop() {
  if (_depth > 0) {
    _depth--;
  }
}

bool badgeNavIsRoot() {
  return _depth <= 1;
}

const char* badgeNavCurrentTitle() {
  if (_depth == 0) return "";
  return _stack[_depth - 1].title;
}

#endif
