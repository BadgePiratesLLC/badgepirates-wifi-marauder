#pragma once
// BSidesKC Badge - real WiFi/Bluetooth on-air state (Nexus c39cd3b3)
//
// "On" means actually associated/active, not "the radio hardware
// exists" - a Marauder spends most of its life scanning with neither
// radio in this state, which is expected, not a fault. Feeds the
// persistent status bar's WiFi/BT glyphs (UI/StatusBar.h) and gates the
// one NTP sync attempt (hardware/ntp_clock.h).

#include <Arduino.h>

// True when actually associated to an AP (WiFi.status()==WL_CONNECTED)
// or running as our own access point (WiFi.softAPIP() set) - either way
// there is a real WiFi link on air right now.
bool wifiIsUp();

// True when the NimBLE stack is initialized (NimBLEDevice::getInitialized()).
// Marauder inits/deinits this around Bluetooth scans and attacks
// (esp32marauder-upstream/esp32_marauder/WiFiScan.cpp) rather than
// leaving it running full-time, so this genuinely toggles during normal use.
bool bluetoothIsUp();
