#pragma once
// BSidesKC Badge - Badge-specific menu items
// Adds LED brightness, buzzer mute, battery status, and hardware test to Marauder menu

#ifdef HAS_SCREEN

void badgeMenuSetup();  // Call after menu_function_obj.RunSetup()

// Nexus 78e62be0 half 2: drive the upstream Menu/MenuNode tree with touch,
// every loop() iteration. Call unconditionally from loop() when HAS_SCREEN -
// it no-ops itself whenever a scan/attack screen (not a menu) owns the
// display. Replaces the old encoder-only "Rotary encoder -> menu
// navigation" block that used to live in main.cpp directly.
void badgeMenuLoop();

#endif
