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
};

extern SimState g_sim;
