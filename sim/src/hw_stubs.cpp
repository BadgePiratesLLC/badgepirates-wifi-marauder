// BSidesKC Badge simulator — trivial stand-ins for the badge peripherals
// that badge_menu.cpp calls through (buzzer, battery, LED ring, encoder,
// power management, backlight). None of these are what this ticket needs
// to prove; they exist so the real badge_menu.cpp links and runs.
//
// sim_counters.h counters are the harness's proof mechanism (see
// sim/README.md) — buzzerPlay() below is the one that matters: it's called
// from both badge_menu.cpp's adaptedActivate() and (in the fake
// MenuFunctions.cpp) upstream's own SELECT path, so its count is the
// "did one tap fire twice" proof Jared asked for ("prove it with a counter
// or a log line, not by eye").

#include "hardware/buzzer.h"
#include "hardware/battery_monitor.h"
#include "hardware/led_feedback.h"
#include "hardware/encoder_handler.h"
#include "hardware/input_test.h"
#include "hardware/power_manager.h"
#include "hardware/display_adapter.h"
#include "sim_counters.h"

// ---- buzzer ----
static bool s_muted = false;
void buzzerInit() {}
void buzzerPlay(BuzzerTone tone) { if (tone == TONE_BUTTON_PRESS) g_sim.ourButtonPressBuzzCount++; }
void buzzerUpdate() {}
void buzzerMute(bool m) { s_muted = m; }
bool buzzerIsMuted() { return s_muted; }

// ---- battery ----
void batteryMonitorInit() {}
void batteryMonitorUpdate(uint32_t) {}
bool batteryIsLow() { return false; }
bool batteryIsCritical() { return false; }
int8_t batteryGetPercent() { return 80; }
float batteryGetVoltage() { return 4.0f; }

// ---- LED ring ----
static uint8_t s_ledBrightness = 33;
void led_feedback_init() {}
void led_feedback_set(LedState) {}
void led_feedback_start_task() {}
void led_feedback_set_brightness(uint8_t b) { s_ledBrightness = b; }
uint8_t led_feedback_get_brightness() { return s_ledBrightness; }

// ---- encoder: driven by the harness's scripted input, not real hardware ----
void encoder_init() {}
bool encoder_turned_up() { return g_sim.consumeEncoderUp(); }
bool encoder_turned_down() { return g_sim.consumeEncoderDown(); }
bool encoder_button_pressed() { return g_sim.consumeEncoderPress(); }

// ---- input test screen: not exercised by this ticket's repro ----
void runInputValidationTest() {}

// ---- power manager ----
void powerManagerInit() {}
void powerManagerUpdate(uint32_t) {}
void powerManagerResetActivity() {}
void powerManagerSleep() {}
bool powerManagerIsDimmed() { return false; }
void powerManagerSetTimeout(uint32_t) {}

// ---- display adapter (backlight + the two bespoke half-1 screens) ----
DisplayAdapter badge_display;
void DisplayAdapter::begin() {}
void DisplayAdapter::backlightOn() {}
void DisplayAdapter::backlightOff() {}
void DisplayAdapter::backlightSet(uint8_t level) { _blLevel = level; }
uint8_t DisplayAdapter::backlightLevel() const { return _blLevel; }
void DisplayAdapter::backlightCycle() {}
void DisplayAdapter::backlightSave() {}
void DisplayAdapter::drawCenteredTitle(const char*) {}
void DisplayAdapter::drawStatusHint(const char*) {}
void DisplayAdapter::drawProgressBar(uint16_t, uint16_t, uint16_t, uint16_t, uint8_t, uint16_t) {}
