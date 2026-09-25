#pragma once
// BSidesKC Badge simulator — harness-visible counters and scripted-input
// queue. Not part of any real firmware API; this is the sim's own proof
// mechanism (Jared, Nexus 78e62be0: "prove it with a counter or a log
// line, not by eye").

#include <cstdint>

struct SimState {
  // Total buzzerPlay(TONE_BUTTON_PRESS) calls since the counter was last
  // reset. badge_menu.cpp's adaptedActivate() calls this on every tap it
  // recognizes; the fake MenuFunctionsSim::main() (sim/fakes/MenuFunctions.cpp)
  // calls it too when upstream's OWN touch path fires a SELECT independent
  // of ours. One physical tap should produce exactly one count.
  int ourButtonPressBuzzCount = 0;

  bool encUp = false, encDown = false, encPress = false;
  bool consumeEncoderUp()    { bool v = encUp;    encUp = false;    return v; }
  bool consumeEncoderDown()  { bool v = encDown;  encDown = false;  return v; }
  bool consumeEncoderPress() { bool v = encPress; encPress = false; return v; }

  // Status bar (Nexus c39cd3b3) - scripted radio/clock state so the
  // harness can render every combination the real bar has to handle
  // (WiFi/BT on vs. off, clock synced vs. never-synced, battery unknown
  // vs. a real reading), not just whatever main.cpp's stubs default to.
  bool wifiUp = false;
  bool bluetoothUp = false;
  bool ntpSynced = false;
  const char* ntpTimeStr = "00:00";
  int8_t battPct = 80;  // -1 = no sense path detected (hw_stubs.cpp mirrors real firmware's meaning)
};

extern SimState g_sim;
