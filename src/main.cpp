// BSidesKC Badge - ESP32Marauder Port
// Phase 1: Minimal bootstrap to verify build system

#include <Arduino.h>
#include "bsideskc_config.h"
#include <TFT_eSPI.h>

TFT_eSPI tft;

void setup() {
    Serial.begin(115200);
    Serial.println("[BSidesKC] Booting...");

    // Backlight on
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);

    // Init display
    tft.init();
    tft.setRotation(1); // landscape
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("BSidesKC Badge");
    tft.println("Marauder Port");
    tft.println("Phase 1 - Build OK");

    Serial.println("[BSidesKC] Display initialized");
}

void loop() {
    delay(1000);
}
