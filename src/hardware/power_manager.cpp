#include "hardware/power_manager.h"
#include "hardware/display_adapter.h"
#include "hardware/led_feedback.h"
#include "hardware/buzzer.h"
#include "bsideskc_pins.h"
#include <esp_sleep.h>

#ifdef HAS_SCREEN
  #include "Display.h"
  extern Display display_obj;
#endif

extern DisplayAdapter badge_display;

static uint32_t _lastActivity = 0;
static uint32_t _sleepTimeout = INACTIVITY_TIMEOUT_MS;
static bool _dimmed = false;
static bool _sleeping = false;

void powerManagerInit() {
  _lastActivity = millis();
  _dimmed = false;
  _sleeping = false;
  Serial.println(F("[Power] Manager initialized"));
}

void powerManagerResetActivity() {
  _lastActivity = millis();
  if (_dimmed) {
    _dimmed = false;
    badge_display.backlightOn();
  }
}

void powerManagerSleep() {
  Serial.println(F("[Power] Entering light sleep..."));
  _sleeping = true;

  // Turn off peripherals
  badge_display.backlightOff();
  led_feedback_set(LED_IDLE);
  buzzerMute(true);

  // Configure wake sources: any button press
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_BOOT, 0);
  gpio_wakeup_enable((gpio_num_t)BTN_ENTER, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)BTN_BACK, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)ENC_BTN_PIN, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  delay(50);
  esp_light_sleep_start();

  // Woke up
  _sleeping = false;
  _lastActivity = millis();
  _dimmed = false;
  buzzerMute(false);
  badge_display.backlightOn();
  led_feedback_init();
  Serial.println(F("[Power] Woke from sleep"));
}

void powerManagerUpdate(uint32_t currentTime) {
  if (_sleeping) return;

  uint32_t elapsed = currentTime - _lastActivity;

  // Dim backlight after inactivity
  if (!_dimmed && elapsed >= BACKLIGHT_DIM_MS) {
    _dimmed = true;
    badge_display.backlightSet(1);  // Minimum brightness
    Serial.println(F("[Power] Backlight dimmed"));
  }

  // Auto-sleep after timeout
  if (elapsed >= _sleepTimeout) {
    powerManagerSleep();
  }
}

bool powerManagerIsDimmed() { return _dimmed; }

void powerManagerSetTimeout(uint32_t ms) { _sleepTimeout = ms; }
