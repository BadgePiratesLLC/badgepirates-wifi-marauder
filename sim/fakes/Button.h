#pragma once
// BSidesKC Badge simulator — fake TFT_eSPI_Button.
//
// Same public surface as the real Extensions/Button.h (contains/press/
// justPressed/justReleased/drawButton), because MenuFunctionsSim.cpp models
// upstream's real menuButton()/buildButtons() logic against it — the whole
// point of this fake is that the *interaction* (three invisible full-width
// "Chicken" zones stacked in thirds, hit-tested every frame) matches what
// MenuFunctions.cpp actually does, not just the type signature.

class TFT_eSPI; // fwd decl, defined earlier in TFT_eSPI.h before this is included
#include "Arduino.h"

class TFT_eSPI_Button {
public:
  TFT_eSPI_Button() {}

  void initButton(TFT_eSPI* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                   uint16_t outline, uint16_t fill, uint16_t textcolor,
                   const char* label, uint8_t textsize) {
    _gfx = gfx;
    _x1 = x - w / 2; _y1 = y - h / 2;
    _w = w; _h = h;
    _outline = outline; _fill = fill; _textcolor = textcolor;
    _label = label ? label : "";
  }
  void initButtonUL(TFT_eSPI* gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
                     uint16_t outline, uint16_t fill, uint16_t textcolor,
                     const char* label, uint8_t textsize) {
    _gfx = gfx; _x1 = x1; _y1 = y1; _w = w; _h = h;
    _outline = outline; _fill = fill; _textcolor = textcolor;
    _label = label ? label : "";
  }

  void setLabelDatum(int16_t, int16_t, uint8_t = 0) {}

  void drawButton(bool inverted = false, String long_name = "") {
    if (!_gfx) return;
    uint16_t fill = inverted ? _textcolor : _fill;
    uint16_t text = inverted ? _fill : _textcolor;
    _gfx->fillRect(_x1, _y1, _w, _h, fill);
    _gfx->setTextColor(text);
    const char* label = long_name.length() ? long_name.c_str() : _label.c_str();
    _gfx->drawString(label, _x1 + 2, _y1 + _h / 2 - 3, 1);
  }

  bool contains(int16_t x, int16_t y) {
    return x >= _x1 && x < (_x1 + (int16_t)_w) && y >= _y1 && y < (_y1 + (int16_t)_h);
  }

  void press(bool p) { laststate = currstate; currstate = p; }
  bool isPressed() { return currstate; }
  bool justPressed() { return currstate && !laststate; }
  bool justReleased() { return !currstate && laststate; }

private:
  TFT_eSPI* _gfx = nullptr;
  int16_t _x1 = 0, _y1 = 0;
  uint16_t _w = 0, _h = 0;
  uint16_t _outline = 0, _fill = 0, _textcolor = 0;
  String _label;
  bool currstate = false, laststate = false;
};
