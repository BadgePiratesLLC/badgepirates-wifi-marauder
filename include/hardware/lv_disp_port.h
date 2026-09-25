#pragma once
// LVGL display bring-up (Nexus 84f4e52c). One function, two bodies:
// src/hardware/lv_disp_port.cpp (real firmware, flushes into the already-
// initialized display_obj.tft) and sim/fakes/lv_disp_port_sim.cpp (native
// sim, flushes into the fake Framebuffer). Same header, same call site in
// badge_menu.cpp - lvDispInit() once at badgeMenuSetup() time.

// Sets up lv_init(), a display sized TFT_WIDTH x TFT_HEIGHT with a
// THEME_LV_DRAW_BUF_BYTES partial-render buffer (no PSRAM on CC13 - see
// badge_ui_theme.h's "LVGL resource budget" section), and a tick source.
// Idempotent: safe to call more than once, only the first call does
// anything.
void lvDispInit();
