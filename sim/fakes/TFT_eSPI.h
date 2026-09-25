#pragma once
// BSidesKC Badge simulator — fake TFT_eSPI.
//
// Implements the subset of the real TFT_eSPI API that our own UI code
// (badge_menu.cpp) and the sim's MenuFunctions model actually call, drawing
// into the shared host Framebuffer instead of an SPI panel. Text uses the
// hand-authored 5x7 font (font5x7.h) — it is legible, not pixel-identical
// to the real GLCD/FreeFont TFT_eSPI renders on the panel. See
// sim/README.md for what that costs us.

#include <cstdint>
#include <cmath>
#include <cstring>
#include "Arduino.h"
#include "Framebuffer.h"
#include "font5x7.h"

// A handful of the named TFT_eSPI colour constants upstream code references.
#define TFT_BLACK     0x0000
#define TFT_WHITE     0xFFFF
#define TFT_RED       0xF800
#define TFT_GREEN     0x07E0
#define TFT_BLUE      0x001F
#define TFT_CYAN      0x07FF
#define TFT_YELLOW    0xFFE0
#define TFT_MAGENTA   0xF81F
#define TFT_LIGHTGREY 0xC618
#define TFT_DARKGREY  0x7BEF

class TFT_eSPI {
public:
  int _rotation = 1;

  void init() {}
  void setRotation(int r) { _rotation = r; }
  int getRotation() const { return _rotation; }
  int width() const { return gFb.w; }
  int height() const { return gFb.h; }

  void fillScreen(uint16_t color) { gFb.fillRect(0, 0, gFb.w, gFb.h, color); }
  void fillRect(int x, int y, int w, int h, uint16_t color) { gFb.fillRect(x, y, w, h, color); }

  void drawFastHLine(int x, int y, int w, uint16_t color) { gFb.fillRect(x, y, w, 1, color); }
  void drawFastVLine(int x, int y, int h, uint16_t color) { gFb.fillRect(x, y, 1, h, color); }

  // Rounded rect: corners are simple quarter-circle cutouts. Good enough to
  // tell "card" from "sharp rect" in a screenshot; not a claim about the
  // panel's actual anti-aliasing.
  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    if (r <= 0) { gFb.fillRect(x, y, w, h, color); return; }
    r = min(r, min(w, h) / 2);
    for (int yy = 0; yy < h; yy++) {
      for (int xx = 0; xx < w; xx++) {
        if (inRoundedRect(xx, yy, w, h, r)) gFb.setPixel(x + xx, y + yy, color);
      }
    }
  }

  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    if (r <= 0) {
      drawFastHLine(x, y, w, color); drawFastHLine(x, y + h - 1, w, color);
      drawFastVLine(x, y, h, color); drawFastVLine(x + w - 1, y, h, color);
      return;
    }
    r = min(r, min(w, h) / 2);
    for (int yy = 0; yy < h; yy++) {
      for (int xx = 0; xx < w; xx++) {
        if (inRoundedRect(xx, yy, w, h, r) && !inRoundedRect(xx, yy, w, h, r, /*shrink=*/1))
          gFb.setPixel(x + xx, y + yy, color);
      }
    }
  }

  void drawCircle(int x0, int y0, int r, uint16_t color) {
    for (int a = 0; a < 360; a++) {
      double rad = a * 3.14159265 / 180.0;
      gFb.setPixel(x0 + (int)(r * cos(rad)), y0 + (int)(r * sin(rad)), color);
    }
  }

  void setTextColor(uint16_t fg) { _fg = fg; _hasBg = false; }
  void setTextColor(uint16_t fg, uint16_t bg) { _fg = fg; _bg = bg; _hasBg = true; }
  void setTextSize(int) {}
  void setFreeFont(const void*) {}
  void setCursor(int x, int y) { _cx = x; _cy = y; }
  void printf(const char*, ...) {}
  void print(const char*) {}
  void drawXBitmap(int, int, const unsigned char*, int, int, uint16_t, uint16_t) {}

  int scaleForFont(int font) const { return font >= 4 ? 3 : (font >= 2 ? 2 : 1); }

  int16_t drawString(const String& s, int x, int y, int font = 1) {
    return drawGlyphs(s.c_str(), x, y, scaleForFont(font));
  }
  int16_t drawCentreString(const String& s, int cx, int y, int font = 1) {
    int scale = scaleForFont(font);
    int w = textWidth(s.c_str(), scale);
    return drawGlyphs(s.c_str(), cx - w / 2, y, scale);
  }
  int16_t textWidth(const char* s, int scale = 1) {
    int n = (int)strlen(s);
    return (int16_t)(n * (6 * scale) - (n > 0 ? scale : 0));
  }

private:
  uint16_t _fg = TFT_WHITE, _bg = TFT_BLACK;
  bool _hasBg = false;
  int _cx = 0, _cy = 0;

  static bool inRoundedRect(int x, int y, int w, int h, int r, int shrink = 0) {
    r -= shrink;
    if (r < 0) return false;
    int left = 0, top = 0, right = w - 1, bottom = h - 1;
    if (x < left + r && y < top + r) { int dx = (left + r) - x, dy = (top + r) - y; return dx * dx + dy * dy <= r * r; }
    if (x > right - r && y < top + r) { int dx = x - (right - r), dy = (top + r) - y; return dx * dx + dy * dy <= r * r; }
    if (x < left + r && y > bottom - r) { int dx = (left + r) - x, dy = y - (bottom - r); return dx * dx + dy * dy <= r * r; }
    if (x > right - r && y > bottom - r) { int dx = x - (right - r), dy = y - (bottom - r); return dx * dx + dy * dy <= r * r; }
    return true;
  }

  int16_t drawGlyphs(const char* s, int x, int y, int scale) {
    int cx = x;
    for (const char* p = s; *p; p++) {
      const uint8_t* rows = font5x7_glyph(*p);
      for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
          bool on = (rows[row] >> (4 - col)) & 1;
          if (on) {
            for (int sy = 0; sy < scale; sy++)
              for (int sx = 0; sx < scale; sx++)
                gFb.setPixel(cx + col * scale + sx, y + row * scale + sy, _fg);
          }
        }
      }
      cx += 6 * scale;
    }
    return (int16_t)(cx - x);
  }
};

#include "Button.h"
