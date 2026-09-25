#pragma once
// BSidesKC Badge - Badge-specific menu items
// Adds LED brightness, buzzer mute, battery status, and hardware test to Marauder menu

#ifdef HAS_SCREEN

void badgeMenuSetup();  // Call after menu_function_obj.RunSetup()

// Nexus 78e62be0 half 2 FAILED on hardware (cards overlaid upstream's own
// menu, taps double/wrong-fired) because badgeMenuLoop() and upstream's own
// MenuFunctions::main() were BOTH called every loop() iteration, both
// reading touch, both drawing to the same panel - main.cpp never suppressed
// either one for the other. Proven in the simulator (Nexus 0f35e128,
// sim/README.md "double-fire / overlay repro"): changeMenu() draws
// upstream's own unstyled button list on every navigation regardless of
// which path opened it, and upstream's own touch-driven SELECT can fire the
// WRONG node entirely (it always activates current_menu->selected, a cursor
// our touch-first UI never moves - so tapping row 2 can silently open
// whatever's at row 0).
//
// True while a menu (not a scan/attack screen) should own the display -
// exactly badgeMenuLoop()'s own criterion, exposed so main.cpp can gate
// upstream's menu_function_obj.main() on the SAME condition instead of
// calling it unconditionally. Exactly one of {badgeMenuLoop(),
// menu_function_obj.main()} should touch the screen/touch controller on
// any given tick; this function is what keeps that true from main.cpp
// without changing a line of upstream's MenuFunctions.cpp.
bool badgeMenuOwnsScreen();

// Drive the upstream Menu/MenuNode tree with touch, every loop() iteration.
// Call unconditionally from loop() when HAS_SCREEN - it no-ops itself
// whenever a scan/attack screen (not a menu) owns the display. Replaces the
// old encoder-only "Rotary encoder -> menu navigation" block that used to
// live in main.cpp directly.
void badgeMenuLoop();

#endif
