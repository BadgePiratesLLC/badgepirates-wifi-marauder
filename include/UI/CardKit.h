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
// double-fire fix (Nexus 78e62be0) is simulator-proven (sim/'s repro/
// doublefire/fixed modes) but NOT hardware-validated - Carla's 78e62be0 QA
// pass at 07:39 UTC on 2026-09-25 recorded the flash attempt itself
// failing ("Failed to connect to ESP32-S3: No serial data received"), so
// it has never actually run on a physical CC13. This port must not risk
// the fix regardless of which state that turns out to be in.
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

// Nexus 84f4e52c deliverable: "render 2 button/card treatments... he picks
// from rendered candidates, then you build the chosen one." Candidate A
// (RaisedGradient) is what actually shipped/flashed while that choice was
// still outstanding - a process gap Nivek caught, not something to hide.
// Candidate B (FlatOutline) exists so Kevin gets the real side-by-side the
// ticket asked for; the sim's "treatmentB" mode (sim/src/sim_main.cpp)
// renders it, real firmware never calls cardkit_set_treatment() so this
// changes zero on-hardware behavior for the build that's already flashed.
enum class CardKitTreatment {
    RaisedGradient = 0,  // shipped: 2-stop vertical gradient, 1px top/bottom bevel, full accent border on select
    FlatOutline = 1,     // candidate B: flat single-shade fill, thin border always on, left accent bar on select
};

// Selects which treatment cardkit_create_card() draws. Default
// RaisedGradient (the shipped/flashed look) so calling this is opt-in.
void cardkit_set_treatment(CardKitTreatment t);

// Builds one card under `parent`. Parent owns the returned lv_obj_t;
// callers repaint by lv_obj_clean(parent) + rebuild, the same
// "clear and rebuild" shape badge_menu.cpp already used for raw tft draws.
lv_obj_t* cardkit_create_card(lv_obj_t* parent, const CardSpec& spec);

// Per-screen title, content-owned (Nexus c39cd3b3): the header band and
// battery glyph this used to draw moved into the persistent
// UI/StatusBar.h so they stop being torn down and rebuilt on every
// repaint; this is what's left - title text, centered, plus the root-only
// "CC13" subtitle line. Reserve THEME_TITLE_H (+ THEME_ROOT_BRAND_H on
// root) below it before drawing the first card row.
lv_obj_t* cardkit_create_title(lv_obj_t* parent, const char* title, bool isRoot);

// Centered hint/placeholder text in the muted colour (e.g. "Nothing here
// yet", the "N-M of T" pager, "Tap a level to apply").
lv_obj_t* cardkit_create_hint(lv_obj_t* parent, const char* text, int16_t centerX, int16_t y);

// LVGL colour from a badge_ui_theme.h RGB565 token - the one conversion
// point between "the theme's numbers" and "what LVGL draws with". Extend
// the theme with new tokens when a new shade is genuinely needed; do not
// hardcode a colour literal anywhere that includes this header.
lv_color_t cardkit_color(uint16_t rgb565);
