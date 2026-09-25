#pragma once
// BSidesKC Badge simulator — shared RGB888 framebuffer the fake TFT_eSPI
// draws into. One global instance (`gFb`) so both the fake TFT and the
// PNG writer in sim_main.cpp see the same pixels.

#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

struct Framebuffer {
  int w, h;
  std::vector<uint8_t> rgb; // w*h*3, row-major, top-to-bottom

  void init(int width, int height) {
    w = width; h = height;
    rgb.assign((size_t)w * h * 3, 0);
  }

  static void rgb565to888(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
    g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
    b = (uint8_t)((c & 0x1F) * 255 / 31);
  }

  void setPixel(int x, int y, uint16_t color565) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    uint8_t r, g, b;
    rgb565to888(color565, r, g, b);
    size_t i = ((size_t)y * w + x) * 3;
    rgb[i] = r; rgb[i + 1] = g; rgb[i + 2] = b;
  }

  void fillRect(int x, int y, int rw, int rh, uint16_t color565) {
    int x0 = max(0, x), y0 = max(0, y);
    int x1 = min(w, x + rw), y1 = min(h, y + rh);
    for (int yy = y0; yy < y1; yy++)
      for (int xx = x0; xx < x1; xx++)
        setPixel(xx, yy, color565);
  }
};

extern Framebuffer gFb;
