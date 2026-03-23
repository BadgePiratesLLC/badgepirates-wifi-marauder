# ESP32Marauder → BSidesKC Badge Porting Plan

## Target Hardware
- **MCU:** ESP32-S3 (dual-core LX7 @ 240MHz)
- **Flash:** 16MB QIO @ 80MHz (board JSON says 8MB N8 — verify actual chip)
- **PSRAM:** Status unclear — board JSON says none, but may be present. Must verify.
- **Display:** ILI9341 320×240 RGB565 SPI (MOSI:11, SCLK:12, CS:10, DC:5, RST:4, BL:6)
- **Touch:** FT6336U capacitive I2C (SDA:8, SCL:9, RST:3, INT:7)
- **WiFi:** Built-in 2.4GHz (promiscuous mode supported on S3)
- **BLE:** BLE 5.0 (NimBLE 2 compatible)
- **SD Card:** SPI (MOSI:35, SCK:36, MISO:37, CS:47) — present on badge
- **NeoPixels:** 6× WS2812B on GPIO 18, 1× status on GPIO 21
- **Buzzer:** GPIO 19
- **Buttons:** Boot(0), Enter(38), Back(39)
- **Rotary Encoder:** A(45), B(48), Button(20)
- **Battery:** MAX17048 fuel gauge on I2C (shared bus with touch)

## Reference
- **Upstream:** https://github.com/justcallmekoko/ESP32Marauder
- **Closest board target:** MARAUDER_V8 (ESP32-S3 + ILI9341 + touch + PSRAM)
- **Badge firmware base:** https://github.com/BadgePiratesLLC/QACode_27

## Phases

### Phase 1: Project Setup & Build System
- Fork or vendor ESP32Marauder source into the repo
- Create platformio.ini for BSidesKC badge (esp32-s3-devkitc-1)
- Create BSIDESKC_BADGE board define in configs.h
- Configure TFT_eSPI User_Setup.h with badge SPI pins
- Get a clean compile (even if non-functional)

### Phase 2: Display & Input
- Verify ILI9341 display works with Marauder's TFT_eSPI rendering
- Replace XPT2046 touch driver with FT6336U I2C capacitive touch
- Create touch adapter that maps FT6336U reads to Marauder's touch interface
- Map hardware buttons (Enter/Back) to Marauder's button interface
- Map rotary encoder to menu scrolling

### Phase 3: WiFi Features
- Verify promiscuous mode works on ESP32-S3
- Test WiFi scanning
- Test packet monitoring
- Test deauthentication
- Test beacon spam
- Test PMKID/EAPOL capture
- Test Evil Portal (AP mode)

### Phase 4: BLE Features
- Configure NimBLE 2 for ESP32-S3
- Test BLE scanning
- Test BLE skimmer detection
- Test BLE spam attacks

### Phase 5: Storage & Persistence
- Configure SD card support (badge HAS SD card on SPI)
- Test PCAP file saves to SD
- Test Evil Portal HTML storage
- Test settings persistence
- Configure SPIFFS as fallback if SD fails

### Phase 6: Badge Integration
- Add NeoPixel feedback (scan active, attack running, etc.)
- Add buzzer alerts
- Add battery level display from MAX17048
- Menu integration with badge firmware (if dual-boot desired)
- OTA update support

### Phase 7: Testing & Polish
- Full feature regression test
- Memory profiling (verify PSRAM usage)
- Power consumption testing
- UI polish for 320×240 landscape

## Key Technical Decisions

1. **Standalone vs integrated:** Start as standalone Marauder firmware (not integrated into badge menu). Can add dual-boot later.
2. **Touch adapter pattern:** Create a thin abstraction layer that translates FT6336U I2C reads into the x/y/pressed format Marauder expects.
3. **PSRAM verification:** First task is to verify if PSRAM is actually present on the badge PCB. This affects buffer sizes significantly.
4. **SD card:** Badge has SD card slot — use it. This gives full PCAP capture capability.

## Pin Mapping Reference

```c
// Display (SPI)
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_CS    10
#define TFT_DC    5
#define TFT_RST   4
#define TFT_BL    6

// Touch (I2C)
#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_RST 3
#define TOUCH_INT 7

// SD Card (SPI)
#define SD_MOSI   35
#define SD_SCK    36
#define SD_MISO   37
#define SD_CS     47

// NeoPixels
#define NEOPIXEL_PIN    18
#define STATUS_LED_PIN  21

// Buzzer
#define BUZZER_PIN 19

// Buttons
#define BTN_BOOT  0
#define BTN_ENTER 38
#define BTN_BACK  39

// Rotary Encoder
#define ENC_A   45
#define ENC_B   48
#define ENC_BTN 20
```
