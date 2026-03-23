#pragma once
// BSidesKC Badge - Battery Monitor
// Wraps upstream BatteryInterface (MAX17048) with low-battery warnings

#include <Arduino.h>

#define LOW_BATTERY_THRESHOLD 15
#define CRITICAL_BATTERY_THRESHOLD 5

void batteryMonitorInit();
void batteryMonitorUpdate(uint32_t currentTime);
bool batteryIsLow();
bool batteryIsCritical();
int8_t batteryGetPercent();
float batteryGetVoltage();
