#pragma once
// BSidesKC Badge - UI Design Tokens
// Nexus 78e62be0: touch-first UI overhaul.
// Single source of truth for color/spacing/radius/type so screens never
// hardcode a raw color or coordinate again. Add new tokens here, not inline.

#include <Arduino.h>

// Panel native resolution. Normally comes from platformio.ini's
// `-D TFT_WIDTH=240 -D TFT_HEIGHT=320` build flags (real firmware) or the
// #ifndef fallback in marauder_config.h (badge_menu.cpp's build, which
// includes configs.h before this header). UI/CardKit.h and its firmware/sim
// lv_disp_port.cpp pair (Nexus 84f4e52c) deliberately do NOT include
// configs.h - the whole point of that hardware seam is staying free of
// upstream Marauder types - so this header, their one shared dependency,
// has to be the one guaranteeing these are defined.
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 320
#endif

// ---- Palette (dark, high-contrast, single accent) ----
#define THEME_BG            0x0000  // near-black background
#define THEME_SURFACE       0x2104  // card / row surface (dark slate)
#define THEME_SURFACE_HI    0x39C7  // pressed/active surface (lighter slate)
#define THEME_TEXT          0xFFFF  // primary text (white)
#define THEME_TEXT_MUTED    0x8410  // secondary/hint text (mid grey)
#define THEME_ACCENT        0x07FF  // single accent - selection/active only (cyan)
#define THEME_OK            0x07E0  // green
#define THEME_WARN          0xFFE0  // yellow
#define THEME_ERROR         0xF800  // red
#define THEME_BORDER        0x4A69  // subtle card border

// ---- Spacing scale (px) ----
#define THEME_SPACE_XS      4
#define THEME_SPACE_SM      8
#define THEME_SPACE_MD      12
#define THEME_SPACE_LG      20

// ---- Corner radius ----
#define THEME_RADIUS_SM     4
#define THEME_RADIUS_MD      8

// ---- Card surface shading (Nexus 84f4e52c: LVGL port, "better buttons") ----
// Card fill is a 2-stop vertical gradient between these, not a flat
// THEME_SURFACE slab - the raised-surface look the ticket asked for. The
// _HI variants are the pressed-state fill (deeper, no highlight), the EDGE_*
// pair is the 1px bevel line pair (top highlight / bottom shade) drawn on
// every card, non-pressed. Derived from THEME_SURFACE/THEME_SURFACE_HI by
// hand (TFT_eSPI RGB565, no runtime colour math available at this layer) -
// keep them here, not recomputed inline, so the bevel moves with the theme.
#define THEME_SURFACE_GRAD_TOP     0x2965  // THEME_SURFACE, +1 shade lighter
#define THEME_SURFACE_GRAD_BOTTOM  0x18C3  // THEME_SURFACE, -1 shade darker
#define THEME_SURFACE_EDGE_HI      0x4A69  // 1px top highlight (== THEME_BORDER)
#define THEME_SURFACE_EDGE_LO      0x0861  // 1px bottom shade, near-black

// ---- Type scale (TFT_eSPI font ids) ----
#define THEME_FONT_SM       1   // ~8px  - subtitles, hints
#define THEME_FONT_MD       2   // ~16px - body / button labels
#define THEME_FONT_LG       4   // large - titles, values

// ---- Chrome layout (derived from screen size, not hand-picked per screen) ----
#define THEME_CARD_MIN_H    44          // spec floor: every tappable row >= 44px tall

// ---- Persistent status bar (Nexus c39cd3b3) ----
// ONE bar, built once under the LVGL screen root and updated in place -
// see UI/StatusBar.h. This replaces the old per-screen THEME_HEADER_H
// band (title + battery glyph, torn down and rebuilt on every repaint);
// the per-screen title moves into content (cardkit_create_title) so the
// bar itself never has to be. Height is the >=44px tap-target floor, not
// a decorative choice - Back and the Settings gear both live here now,
// so THEME_CARD_MIN_H's tap-target rule extends to chrome too.
#define THEME_STATUSBAR_H   44
#define THEME_TITLE_H       22          // per-screen title row, content-owned now (cardkit_create_title)
#define THEME_BACK_W        48          // Back control width, icon-only (was "< Back" text) - frees room for the status cluster
#define THEME_BACK_H        THEME_STATUSBAR_H
#define THEME_GEAR_W        44          // Settings gear, right edge, same tap-target floor as Back
#define THEME_BATT_W        22          // battery shell width, status-cluster icon
#define THEME_BATT_H        12
#define THEME_ROOT_BRAND_H  16          // device-name strip under the title, root screen only

// ---- Motion (restraint: instant feedback, no spinners) ----
// Pressed state is drawn synchronously on touch-down; there is no
// animation budget beyond that by design. Kept as a named constant so
// nobody adds a delay() "for a nice fade" later.
#define THEME_NO_ARTIFICIAL_DELAY 0

// ---- LVGL resource budget (Nexus 84f4e52c) ----
// CC13 is 8MB flash, NO PSRAM (src/main.cpp:205, disabled after an
// RTC_SW_SYS_RST boot loop) - every byte here comes out of ~320KB of
// internal SRAM shared with WiFi/BLE. CC15 is 16MB WITH PSRAM
// (bp_cc15_n16r2). Per Jared's accommodation on this ticket: keep these as
// named constants in ONE place so a richer CC15 build is a constants change,
// not a rewrite - do not copy these values inline anywhere else.
#define THEME_LV_MEM_SIZE_BYTES   (64 * 1024)          // lv_conf.h LV_MEM_SIZE
#define THEME_LV_DRAW_BUF_DIVISOR 10                    // 1/10 screen, partial render (matches CC15's proven draw_buf sizing)
#define THEME_LV_DRAW_BUF_BYTES   (TFT_WIDTH * TFT_HEIGHT / THEME_LV_DRAW_BUF_DIVISOR * 2)
