#pragma once
// BSidesKC Badge simulator — fake Display (stands in for upstream's
// esp32marauder-upstream/esp32_marauder/Display.h).
//
// Provides display_obj.tft / .key[] / .updateTouch() with the same shapes
// badge_menu.cpp and the sim's MenuFunctions model use. Touch state is fed
// by the harness (sim_set_touch) instead of a real digitizer.

#include "Arduino.h"
#include "TFT_eSPI.h"

#define BUTTON_ARRAY_LEN 12 // == BUTTON_SCREEN_LIMIT, marauder_config.h

// Scan-mode constants badge_menu.cpp/MenuFunctions reference (real values
// for WIFI_SCAN_OFF/OTA_UPDATE/SHOW_INFO come from upstream Display.h;
// WIFI_CONNECTED/ESP_UPDATE just need to be distinct sentinels here since
// the sim never drives a real WiFi connect).
#define WIFI_SCAN_OFF  0
#define OTA_UPDATE     100
#define SHOW_INFO      101
#define WIFI_CONNECTED 200
#define ESP_UPDATE     201

struct WiFiScan {
  int currentScanMode = WIFI_SCAN_OFF;
};
extern WiFiScan wifi_scan_obj;

void sim_set_touch(bool down, uint16_t x, uint16_t y);

class Display {
public:
  TFT_eSPI tft;
  TFT_eSPI_Button key[BUTTON_ARRAY_LEN + 4];
  const String version_number = "sim";
  bool exit_draw = false;
  bool headless_mode = false;

  void RunSetup() { tft.init(); tft.setRotation(1); }
  void init() { /* real one resets scroll state; no-op here */ }
  void clearScreen() { tft.fillScreen(TFT_BLACK); }
  void displayBuffer(bool = false) {}
  void updateBanner(const String&) {}

  uint8_t updateTouch(uint16_t* x, uint16_t* y, uint16_t = 600);

  // Real signature: MenuFunctions.cpp:14 — hit-tests the three invisible
  // full-width "Chicken" zones buildButtons() lays across the screen in
  // thirds (key[BUTTON_ARRAY_LEN..+3]) and returns which one just released,
  // or -1. See sim/fakes/MenuFunctions.cpp for why this matters.
  int8_t menuButton(uint16_t* x, uint16_t* y, bool pressed, bool check_hold = false) {
    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++)
      key[b].press(pressed && key[b].contains(*x, *y));
    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (!check_hold) {
        if (key[b].justReleased() && !pressed) return b - BUTTON_ARRAY_LEN;
      } else if (key[b].isPressed()) {
        return b - BUTTON_ARRAY_LEN;
      }
    }
    return -1;
  }
};
