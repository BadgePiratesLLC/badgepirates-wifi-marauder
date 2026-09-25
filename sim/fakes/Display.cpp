#include "Display.h"

static bool s_touchDown = false;
static uint16_t s_touchX = 0, s_touchY = 0;

void sim_set_touch(bool down, uint16_t x, uint16_t y) {
  s_touchDown = down; s_touchX = x; s_touchY = y;
}

uint8_t Display::updateTouch(uint16_t* x, uint16_t* y, uint16_t) {
  if (s_touchDown) { *x = s_touchX; *y = s_touchY; return 1; }
  return 0;
}
