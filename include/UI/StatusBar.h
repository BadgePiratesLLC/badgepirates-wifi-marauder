#pragma once
// Persistent status bar for the CC13 badge UI (Nexus c39cd3b3).
//
// ONE lv_obj_t, built once under the LVGL screen root and updated in
// place - not the per-screen "clear and rebuild" pattern CardKit.h's
// header comment describes for content. This is exactly the chrome vs.
// content split that fixes it: badge_menu.cpp's lvScreen() builds this
// bar a single time; every screen repaint below it only touches a
// separate content container, never this bar. See badge_menu.cpp's
// lvRepaintBegin()/lvRepaintEnd() for the split.
//
// Same hardware seam as UI/CardKit.h: pure LVGL + badge_ui_theme.h,
// no upstream Marauder types directly in this file. It DOES call the
// project's own hardware/battery_monitor.h, hardware/radio_status.h and
// hardware/ntp_clock.h accessors (see StatusBar.cpp) - those have
// sim-side stand-ins in sim/src/hw_stubs.cpp with identical signatures,
// same pattern badge_menu.cpp already relies on for batteryGetPercent().
#include <lvgl.h>

// Builds the bar as a child of `parent` (the persistent LVGL screen
// object from badge_menu.cpp's lvScreen()). Call exactly once.
void statusbar_create(lv_obj_t* parent);

// Back visibility/pressed-state and the gear's pressed-state. Cheap:
// edits the two existing button objects in place, does not rebuild them.
// Callers already have this information at every repaint (root or not,
// what's currently pressed) - see badge_menu.cpp's lvRepaintBegin().
void statusbar_setChrome(bool showBack, bool backPressed, bool gearPressed);

// Battery/WiFi/BT/clock glyphs. Call every badgeMenuLoop() tick, not
// just on repaint - these can change with no navigation at all (WiFi
// associates while the user just sits on a menu). Cheap: battery is
// internally throttled to a slow poll (30s+, it's the one real I2C
// read); WiFi/BT/clock reads are free state checks. Every glyph is
// diffed against its last-drawn value before touching LVGL - "update on
// state change, not a timer," even though this itself IS called every
// tick.
void statusbar_pollIndicators(uint32_t currentTime);
