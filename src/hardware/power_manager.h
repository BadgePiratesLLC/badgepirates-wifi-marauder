#pragma once
// BSidesKC Badge - Power Management
// Sleep mode, auto-sleep on inactivity, wake-on-button

#include <Arduino.h>

#define INACTIVITY_TIMEOUT_MS  (5 * 60 * 1000)  // 5 minutes default
#define BACKLIGHT_DIM_MS       (2 * 60 * 1000)  // Dim after 2 minutes

void powerManagerInit();
void powerManagerUpdate(uint32_t currentTime);
void powerManagerResetActivity();  // Call on any user input
void powerManagerSleep();          // Enter light sleep immediately
bool powerManagerIsDimmed();
void powerManagerSetTimeout(uint32_t ms);
