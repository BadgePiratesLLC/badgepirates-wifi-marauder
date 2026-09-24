#pragma once
// BSidesKC Badge - UI Design Tokens
// Nexus 78e62be0: touch-first UI overhaul.
// Single source of truth for color/spacing/radius/type so screens never
// hardcode a raw color or coordinate again. Add new tokens here, not inline.

#include <Arduino.h>

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

// ---- Type scale (TFT_eSPI font ids) ----
#define THEME_FONT_SM       1   // ~8px  - subtitles, hints
#define THEME_FONT_MD       2   // ~16px - body / button labels
#define THEME_FONT_LG       4   // large - titles, values

// ---- Chrome layout (derived from screen size, not hand-picked per screen) ----
#define THEME_HEADER_H      28          // header bar height (title + battery live here)
#define THEME_BACK_W        64          // Back control width
#define THEME_BACK_H        (THEME_HEADER_H - 4)
#define THEME_CARD_MIN_H    44          // spec floor: every tappable row >= 44px tall
#define THEME_BATT_W        30          // battery glyph width, top-right corner, every screen
#define THEME_BATT_H        14

// ---- Motion (restraint: instant feedback, no spinners) ----
// Pressed state is drawn synchronously on touch-down; there is no
// animation budget beyond that by design. Kept as a named constant so
// nobody adds a delay() "for a nice fade" later.
#define THEME_NO_ARTIFICIAL_DELAY 0
