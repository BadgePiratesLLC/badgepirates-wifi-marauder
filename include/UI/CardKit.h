#pragma once
// Card/chrome primitives for the CC13 badge UI (Nexus 84f4e52c: LVGL port,
// "better buttons").
//
// Pure LVGL + badge_ui_theme.h token values - no TFT_eSPI, no upstream
// Marauder types. That is the hardware seam that lets the exact same
// compiled code drive both the panel (src/hardware/lv_disp_port.cpp's
// flush_cb into display_obj.tft) and the native sim/ screenshot tool
// (sim/fakes/lv_disp_port_sim.cpp's flush_cb into the fake Framebuffer) -
// the same pattern CC15's QACode_27/include/UI/CardKit.h already proved
// out on real hardware, adapted here for CC13's portrait card-list layout
// instead of CC15's button grid.
//
// Rendering model: every function here BUILDS lv_obj_t children under a
// caller-owned parent and returns immediately - nothing here pumps
// lv_timer_handler or flushes a frame. Callers (badge_menu.cpp) call
// lv_refr_now() once after building/rebuilding a screen, which is what
// actually pushes pixels - "instant, no animation budget" per
// THEME_NO_ARTIFICIAL_DELAY. This keeps badge_menu.cpp's existing
// blocking-loop control flow and TapDetector-based touch hit-testing
// completely unchanged; only HOW a screen's pixels get drawn changed, not
// when/how touch is read or what fires on a tap. That adapter's
// double-fire fix (Nexus 78e62be0) is hardware-validated and this port
// must not risk it.
#include <lvgl.h>
#include "badge_ui_theme.h"

// A tappable card row: rounded corners actually drawn (THEME_RADIUS_MD was
// declared and ignored before this), 2-stop vertical gradient fill, 1px
// top-highlight/bottom-shade bevel so it reads as a raised surface, a real
// inset pressed state (deeper flat fill + no highlight, not just a colour
// swap), and an optional trailing chevron for rows that navigate deeper.
// Every field maps to one bullet in Nexus 84f4e52c's "what better buttons
// means concretely" list.
struct CardSpec {
    int16_t x, y, w, h;
    const char* title;
    const char* subtitle;  // nullptr = no second line
    bool selected;          // accent border, e.g. current option value
    bool pressed;           // real pressed state: deeper fill, bevel hidden
    bool navigable;         // draw the trailing chevron
};

// Builds one card under `parent`. Parent owns the returned lv_obj_t;
// callers repaint by lv_obj_clean(parent) + rebuild, the same
// "clear and rebuild" shape badge_menu.cpp already used for raw tft draws.
lv_obj_t* cardkit_create_card(lv_obj_t* parent, const CardSpec& spec);

// Header band: title, optional "CC13" subtitle line (root only), battery
// glyph pinned top-right. Matches THEME_HEADER_H/THEME_BATT_W/THEME_BATT_H.
lv_obj_t* cardkit_create_header(lv_obj_t* parent, const char* title, bool isRoot, int batteryPct);

// Persistent Back control, fixed geometry (THEME_BACK_W x THEME_BACK_H at
// 0,0) matching badge_menu.cpp's kBackRect hit-test zone - this only ever
// needs to be repainted to match a pressed state, never repositioned.
lv_obj_t* cardkit_create_back_button(lv_obj_t* parent, bool pressed);

// Centered hint/placeholder text in the muted colour (e.g. "Nothing here
// yet", the "N-M of T" pager, "Tap a level to apply").
lv_obj_t* cardkit_create_hint(lv_obj_t* parent, const char* text, int16_t centerX, int16_t y);

// LVGL colour from a badge_ui_theme.h RGB565 token - the one conversion
// point between "the theme's numbers" and "what LVGL draws with". Extend
// the theme with new tokens when a new shade is genuinely needed; do not
// hardcode a colour literal anywhere that includes this header.
lv_color_t cardkit_color(uint16_t rgb565);
