#pragma once
// BSidesKC Badge → ESP32 Marauder Board Configuration
// Based on MARAUDER_V8 with badge-specific pin overrides.
// This file is included INSTEAD of upstream configs.h board sections.
// Define the upstream guard so configs.h becomes a no-op when included.
#ifndef configs_h
#define configs_h
#endif

#include "bsideskc_pins.h"
#include "bsideskc_config.h"

// ---- Board Identity ----
#define BSIDESKC_BADGE
#define MARAUDER_V8  // upstream .cpp files check this for V8-specific code paths
#define HARDWARE_NAME "BSidesKC Badge"
#define MARAUDER_VERSION "v1.11.0"

// ---- Upstream Compat Constants ----
#define POLISH_POTATO
#define JSON_SETTING_SIZE   2048
#define GRAPH_REFRESH       100
#define TRACK_EVICT_SEC     90
#define DUAL_BAND_CHANNELS  51
#define DISPLAY_BUFFER_LIMIT 20

#define MODE_OFF     0
#define MODE_RAINBOW 1
#define MODE_ATTACK  2
#define MODE_SNIFF   3
#define MODE_CUSTOM  4

// ---- Feature Flags (V8 baseline, badge adjustments) ----
// Matches V8: HAS_TOUCH, HAS_SCREEN, HAS_FULL_SCREEN, HAS_BT,
//   HAS_SD, USE_SD, HAS_PSRAM, HAS_IDF_3, HAS_GPS, HAS_SEPARATE_SD
// Badge adds: HAS_NEOPIXEL_LED, HAS_BUTTONS, HAS_BATTERY
// Badge removes: HAS_DUAL_BAND (single-band ESP32-S3), HAS_NIMBLE_2 (using NimBLE 1.4.x)
// #define HAS_PSRAM  // badge has no PSRAM — ps_malloc returns NULL → StoreProhibited crash
#undef HAS_PSRAM
#ifdef HAS_PSRAM
#error "HAS_PSRAM is still defined! Config override failed!"
#endif
#define HAS_GPS
#define HAS_SEPARATE_SD
// HAS_DUAL_BAND intentionally omitted — badge is single-band ESP32-S3

// ---- Touch: FT6336U via XPT2046 shim (see include/XPT2046_Touchscreen.h) ----
#define HAS_CYD_TOUCH
// Dummy XPT2046 pin values — the shim ignores them but upstream code references them
#define XPT2046_IRQ   -1
#define XPT2046_MOSI  -1
#define XPT2046_MISO  -1
#define XPT2046_CLK   -1
#define XPT2046_CS    -1

// ---- Display Pin Mapping (Marauder names ← badge pins) ----
#define TFT_MOSI    TFT_MOSI_PIN   // 11
#define TFT_SCLK    TFT_SCLK_PIN   // 12
#define TFT_CS      TFT_CS_PIN     // 10
#define TFT_DC      TFT_DC_PIN     // 5
#define TFT_RST     TFT_RST_PIN    // 4
#define TFT_BL      TFT_BL_PIN     // 6
#define TOUCH_CS    -1              // I2C touch, no SPI CS

#define HAS_ILI9341
#define CHAN_PER_PAGE       7
#define SCREEN_CHAR_WIDTH   40
#define BANNER_TEXT_SIZE    2

#ifndef TFT_WIDTH
  #define TFT_WIDTH  240
#endif
#ifndef TFT_HEIGHT
  #define TFT_HEIGHT 320
#endif

#define GRAPH_VERT_LIM      (TFT_HEIGHT / 2 - 1)
#define EXT_BUTTON_WIDTH    30
#define SCREEN_BUFFER
#define MAX_SCREEN_BUFFER   21
#define SCREEN_ORIENTATION  1  // Landscape (320×240)

#define CHAR_WIDTH          12
#define SCREEN_WIDTH        TFT_WIDTH
#define SCREEN_HEIGHT       TFT_HEIGHT
#define HEIGHT_1            TFT_WIDTH
#define WIDTH_1             TFT_HEIGHT
#define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH / 6)
#define TEXT_HEIGHT         16
#define BOT_FIXED_AREA      0
#define TOP_FIXED_AREA      48
#define YMAX                320
#define minimum(a, b)       (((a) < (b)) ? (a) : (b))
#define MENU_FONT           &FreeMono9pt7b
#define BUTTON_SCREEN_LIMIT 12
#define BUTTON_ARRAY_LEN    BUTTON_SCREEN_LIMIT
#define STATUS_BAR_WIDTH    16
#define LVGL_TICK_PERIOD    6
#define STATUSBAR_COLOR     0x4A49

#define FRAME_X  100
#define FRAME_Y  64
#define FRAME_W  120
#define FRAME_H  50
#define REDBUTTON_X   FRAME_X
#define REDBUTTON_Y   FRAME_Y
#define REDBUTTON_W   (FRAME_W / 2)
#define REDBUTTON_H   FRAME_H
#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W / 2)
#define GREENBUTTON_H FRAME_H

// ---- Menu Definitions (matches V8) ----
#define BANNER_TIME     100
#define COMMAND_PREFIX  "!"
#define KEY_X           120
#define KEY_Y           50
#define KEY_W           240
#define KEY_H           22
#define KEY_SPACING_X   0
#define KEY_SPACING_Y   1
#define KEY_TEXTSIZE    1
#define ICON_W          22
#define ICON_H          22
#define BUTTON_PADDING  22

// ---- SD Card (badge has dedicated SPI bus) ----
#define SD_CS   SD_CS_PIN    // 47
#define SD_MISO SD_MISO_PIN  // 37
#define SD_MOSI SD_MOSI_PIN  // 35
#define SD_SCK  SD_SCK_PIN   // 36

// ---- NeoPixel ----
#define PIN NEOPIXEL_PIN     // 18
#define Pixels NEOPIXEL_COUNT // 6

// ---- GPS (V8 defaults, adjust when hardware confirmed) ----
#define GPS_SERIAL_INDEX 1
#define GPS_TX 14
#define GPS_RX 13

// ---- Battery (I2C fuel gauge on shared touch bus) ----
#define I2C_SDA I2C_SDA_PIN  // 8
#define I2C_SCL I2C_SCL_PIN  // 9

// ---- Buttons (badge uses rotary encoder + buttons, not V8 touch-only) ----
#define L_BTN -1
#define C_BTN BTN_ENTER      // 38
#define U_BTN -1
#define R_BTN -1
#define D_BTN BTN_BACK       // 39
#define HAS_C
#define HAS_D
#define L_PULL true
#define C_PULL true
#define U_PULL true
#define R_PULL true
#define D_PULL true

// ---- Memory ----
#define MEM_LOWER_LIM 10000
#define KIT_LED_BUILTIN 13

// ---- PCAP Buffer (PSRAM available) ----
#define BUF_SIZE  (8 * 1024)
#define SNAP_LEN  (1 * 4096)

// ---- Evil Portal ----
// Reduced from 30000 to 8192: saves ~22KB BSS for memory-constrained ESP32-S3 without PSRAM
#define MAX_HTML_SIZE 8192

// ---- MAC History (PSRAM available) ----
#define mac_history_len      50
#define mac_history_len_half (mac_history_len / 2)

// ---- Marauder Title ----
#define MARAUDER_TITLE_BYTES 13578

// ---- WiFi ----
#define HOP_DELAY 1000

// ---- Space-Saving Colors (upstream compat) ----
#define TFTWHITE     1
#define TFTCYAN      2
#define TFTBLUE      3
#define TFTRED       4
#define TFTGREEN     5
#define TFTGREY      6
#define TFTGRAY      7
#define TFTMAGENTA   8
#define TFTVIOLET    9
#define TFTORANGE    10
#define TFTYELLOW    11
#define TFTLIGHTGREY 12
#define TFTPURPLE    13
#define TFTNAVY      14
#define TFTSILVER    15
#define TFTDARKGREY  16
#define TFTSKYBLUE   17
#define TFTLIME      18
#define TFT_FARTGRAY 0x528a
