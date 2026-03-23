#pragma once
// BSidesKC Badge - Comprehensive Input Validation Test Mode
// Exercises all Phase 2 input components on a single screen.
// Enter by holding BOOT (GPIO 0) during power-on.

#include <Arduino.h>

// Run the full input validation test. Blocks until user exits.
void runInputValidationTest();
