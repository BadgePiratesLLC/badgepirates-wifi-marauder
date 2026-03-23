#include "hardware/led_feedback.h"
#include "bsideskc_pins.h"

static Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel statusLed(STATUS_LED_COUNT, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);
static uint8_t ledBrightness = 33;

static LedState currentState = LED_IDLE;
static LedState prevState = LED_IDLE;
static uint32_t lastUpdate = 0;
static uint32_t flashStart = 0;
static const uint32_t FLASH_DURATION = 600;

void led_feedback_init() {
    pixels.begin();
    pixels.setBrightness(33);
    pixels.clear();
    pixels.show();
    statusLed.begin();
    statusLed.setBrightness(50);
    statusLed.clear();
    statusLed.show();
}

void led_feedback_set(LedState state) {
    if (state == LED_CAPTURE_OK || state == LED_ERROR) {
        prevState = currentState;
        flashStart = millis();
    }
    currentState = state;
}

// Breathing: ramp brightness 10..80 using sine-ish triangle
static void pattern_idle() {
    uint8_t phase = (millis() / 16) & 0xFF;
    uint8_t b = (phase < 128) ? phase : (255 - phase);
    b = 10 + (b * 70 / 128);
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        pixels.setPixelColor(i, pixels.Color(0, b / 3, b));
    statusLed.setPixelColor(0, statusLed.Color(0, b / 3, b));
}

// Rotating single bright pixel
static void pattern_scanning() {
    int pos = (millis() / 120) % NEOPIXEL_COUNT;
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        pixels.setPixelColor(i, (i == pos) ? pixels.Color(0, 0, 180) : pixels.Color(0, 0, 20));
    statusLed.setPixelColor(0, statusLed.Color(0, 0, 255));
}

// Pulsing red
static void pattern_attack() {
    uint8_t phase = (millis() / 8) & 0xFF;
    uint8_t b = (phase < 128) ? phase : (255 - phase);
    b = 40 + (b * 215 / 128);
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        pixels.setPixelColor(i, pixels.Color(b, 0, 0));
    statusLed.setPixelColor(0, statusLed.Color(255, 0, 0));
}

// Green flash
static void pattern_capture_ok() {
    bool on = ((millis() / 80) & 1) == 0;
    uint32_t c = on ? pixels.Color(0, 255, 0) : 0;
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        pixels.setPixelColor(i, c);
    statusLed.setPixelColor(0, on ? statusLed.Color(0, 255, 0) : 0);
}

// Red blink
static void pattern_error() {
    bool on = ((millis() / 150) & 1) == 0;
    uint32_t c = on ? pixels.Color(255, 0, 0) : 0;
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        pixels.setPixelColor(i, c);
    statusLed.setPixelColor(0, on ? statusLed.Color(255, 0, 0) : 0);
}

void led_feedback_update() {
    // Rate-limit to ~30fps
    uint32_t now = millis();
    if (now - lastUpdate < 33) return;
    lastUpdate = now;

    // Auto-return from transient states
    if ((currentState == LED_CAPTURE_OK || currentState == LED_ERROR) &&
        (now - flashStart > FLASH_DURATION)) {
        currentState = prevState;
    }

    switch (currentState) {
        case LED_IDLE:       pattern_idle(); break;
        case LED_SCANNING:   pattern_scanning(); break;
        case LED_ATTACK:     pattern_attack(); break;
        case LED_CAPTURE_OK: pattern_capture_ok(); break;
        case LED_ERROR:      pattern_error(); break;
    }

    pixels.show();
    statusLed.show();
}

void led_feedback_set_brightness(uint8_t brightness) {
    ledBrightness = brightness;
    pixels.setBrightness(brightness);
    statusLed.setBrightness(brightness);
}

uint8_t led_feedback_get_brightness() {
    return ledBrightness;
}
