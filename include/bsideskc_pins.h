#pragma once
// BSidesKC Badge Pin Definitions
// Source: PORTING_PLAN.md

// Display (SPI) - ILI9341
#define TFT_MOSI_PIN  11
#define TFT_SCLK_PIN  12
#define TFT_CS_PIN    10
#define TFT_DC_PIN    5
#define TFT_RST_PIN   4
#define TFT_BL_PIN    6

// Touch (I2C) - FT6336U
#define TOUCH_SDA_PIN 8
#define TOUCH_SCL_PIN 9
#define TOUCH_RST_PIN 3
#define TOUCH_INT_PIN 7

// SD Card (SPI)
#define SD_MOSI_PIN   35
#define SD_SCK_PIN    36
#define SD_MISO_PIN   37
#define SD_CS_PIN     47

// NeoPixels
#define NEOPIXEL_PIN      18
#define NEOPIXEL_COUNT    6
#define STATUS_LED_PIN    21
#define STATUS_LED_COUNT  1

// Buzzer
#define BUZZER_PIN 19

// Buttons
#define BTN_BOOT  0
#define BTN_ENTER 38
#define BTN_BACK  39

// Rotary Encoder
#define ENC_A_PIN   45
#define ENC_B_PIN   48
#define ENC_BTN_PIN 20

// I2C (shared: touch + MAX17048 fuel gauge)
#define I2C_SDA_PIN TOUCH_SDA_PIN
#define I2C_SCL_PIN TOUCH_SCL_PIN
