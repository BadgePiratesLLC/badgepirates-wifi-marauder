#pragma once
// BSidesKC Badge - Screen back-stack (Nexus 78e62be0)
//
// One generic navigation primitive for touch screens we own (badge_menu.cpp
// today; the desk-console app framework in Nexus 7ea89cbf is expected to
// reuse this rather than inventing a second stack). A "screen" is just a
// void() function that draws itself and runs its own input-poll loop; Back
// pops the stack and re-invokes whatever was underneath.

#include <Arduino.h>

#ifdef HAS_SCREEN

typedef void (*BadgeScreenFn)();

// Registers the screen currently on top of the stack (call at the start of
// a screen's render function, before its poll loop, so a Back tap from a
// screen further down knows what "current title" to show if needed).
void badgeNavPush(BadgeScreenFn fn, const char* title);

// Pops the current screen and re-invokes the one beneath it, if any.
// Screens themselves still `return;` out of their own poll loop on Back —
// this just keeps the stack (and title) bookkeeping in one place.
void badgeNavPop();

// True when the stack has nothing under the current screen (i.e. this is
// the root/idle screen) - used to decide whether to draw a Back control.
bool badgeNavIsRoot();

const char* badgeNavCurrentTitle();

#endif
