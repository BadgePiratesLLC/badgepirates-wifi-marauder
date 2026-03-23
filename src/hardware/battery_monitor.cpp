#include "hardware/battery_monitor.h"
#include "hardware/buzzer.h"
#include "BatteryInterface.h"

#ifdef HAS_SCREEN
  #include "Display.h"
  extern Display display_obj;
#endif

extern BatteryInterface battery_obj;

static uint32_t _lastWarnTime = 0;
static const uint32_t WARN_INTERVAL_MS = 30000;
static bool _lowWarned = false;

void batteryMonitorInit() {
  // Upstream battery_obj.RunSetup() already called in main setup()
  Serial.println(F("[Battery] Monitor initialized"));
}

void batteryMonitorUpdate(uint32_t currentTime) {
  int8_t pct = battery_obj.battery_level;
  if (pct < 0) return;  // no valid reading

  if (pct <= CRITICAL_BATTERY_THRESHOLD) {
    if (currentTime - _lastWarnTime >= WARN_INTERVAL_MS) {
      _lastWarnTime = currentTime;
      buzzerPlay(TONE_LOW_BATTERY);
      Serial.println(F("[Battery] CRITICAL - charge now!"));
      #ifdef HAS_SCREEN
        display_obj.tft.fillRect(0, 0, TFT_WIDTH, STATUS_BAR_WIDTH, TFT_RED);
        display_obj.tft.setTextColor(TFT_WHITE, TFT_RED);
        display_obj.tft.drawCentreString("LOW BATTERY!", TFT_WIDTH / 2, 0, 2);
      #endif
    }
  } else if (pct <= LOW_BATTERY_THRESHOLD && !_lowWarned) {
    _lowWarned = true;
    _lastWarnTime = currentTime;
    buzzerPlay(TONE_LOW_BATTERY);
    Serial.printf("[Battery] Low: %d%%\n", pct);
  } else if (pct > LOW_BATTERY_THRESHOLD) {
    _lowWarned = false;
  }
}

bool batteryIsLow() {
  return battery_obj.battery_level >= 0 && battery_obj.battery_level <= LOW_BATTERY_THRESHOLD;
}

bool batteryIsCritical() {
  return battery_obj.battery_level >= 0 && battery_obj.battery_level <= CRITICAL_BATTERY_THRESHOLD;
}

int8_t batteryGetPercent() {
  return battery_obj.battery_level;
}

float batteryGetVoltage() {
  // MAX17048 voltage via upstream Adafruit lib
  if (battery_obj.has_max17048) {
    extern Adafruit_MAX17048 maxlipo;
    // Access through battery_obj not possible (private), return estimate
    return battery_obj.battery_level * 0.042f;  // rough 4.2V max estimate
  }
  return -1.0f;
}
