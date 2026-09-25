#pragma once
// BSidesKC Badge - best-effort NTP clock for the status bar (Nexus c39cd3b3)
//
// This is a Marauder build: it spends its life scanning, in promiscuous
// mode, or running as an AP - rarely associated to an AP with real
// internet - and there is no battery-backed RTC (src/main.cpp includes
// rom/rtc.h only, which does not survive power loss). So: never block
// boot on this, and never report a time unless the clock was actually
// set from a real NTP reply. A wrong clock on a security tool is a
// credibility problem, and this badge goes to people who will notice.

#include <Arduino.h>

// Call every loop() tick with the current WiFi-association state
// (hardware/radio_status.h's wifiIsUp()). Cheap when not associated -
// does nothing until WiFi comes up, and only fires ONE non-blocking
// configTzTime() call per association (not every tick).
void ntpClockUpdate(bool wifiAssociated, uint32_t currentTime);

// True once the system clock reads a real, post-2024 time - i.e. it was
// actually set by an NTP reply, not just "we asked." configTime() itself
// returns immediately whether or not the request ever completes, so
// "we called it" is not the same thing as "it worked."
bool ntpIsSynced();

// Fills buf with "HH:MM" (24h, local to THEME_NTP_TZ below). Only
// meaningful when ntpIsSynced() is true - callers must check that first
// and hide the clock entirely otherwise, never show a stale/zero time.
void ntpGetTimeString(char* buf, size_t bufLen);
