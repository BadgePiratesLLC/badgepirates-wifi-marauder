# Hardware Verification — BSidesKC Badge

## Build Verification Checklist (Phase 1)

### Completed ✅
- [x] PlatformIO project compiles for ESP32-S3 target (`esp32-s3-devkitc-1`)
- [x] `platformio.ini` configured: 16MB flash, QIO @ 80MHz, correct partition table
- [x] All library dependencies resolve (TFT_eSPI, FT6336U, NimBLE, NeoPixel, MAX1704X, ArduinoJson, LinkedList, ESP32Ping, AsyncTCP, ESP Async WebServer, MicroNMEA, EspSoftwareSerial)
- [x] `BSIDESKC_BADGE` board define created with full pin mapping
- [x] `bsideskc_pins.h` — all 25+ GPIO assignments defined, no conflicts
- [x] `bsideskc_config.h` — feature flags set (HAS_SCREEN, HAS_BT, HAS_SD, HAS_NEOPIXEL_LED, etc.)
- [x] `marauder_config.h` — full upstream compatibility layer mapping badge pins → Marauder names
- [x] TFT_eSPI configured via build flags (no User_Setup.h modification needed)
- [x] ESP32Marauder vendored as git submodule at pinned commit
- [x] `configs.h` shim redirects upstream includes to `marauder_config.h`
- [x] No GPIO pin conflicts across display, touch, SD, buttons, encoder, NeoPixels, buzzer
- [x] Upstream Marauder `.cpp` sources compiled via `build_src_filter` (excluding `.ino`)
- [x] Full `main.cpp` bootstrap replicates upstream `esp32_marauder.ino` initialization sequence
- [x] All upstream global objects instantiated (WiFiScan, EvilPortal, Buffer, Settings, CommandLine, Display, MenuFunctions, LedInterface, SDInterface, GpsInterface, BatteryInterface)
- [x] Backlight brightness control with persistent preferences
- [x] `lib_ldf_mode = deep+` resolves all transitive upstream dependencies

### Verified in Simulation (No Hardware)
- [x] All `#ifdef BSIDESKC_BADGE` code paths compile correctly
- [x] Pin mapping consistency: `bsideskc_pins.h` → `marauder_config.h` → `platformio.ini` build flags all agree
- [x] MARAUDER_V8 compatibility: all upstream-expected `#define`s present in `marauder_config.h`
- [x] Feature flag coverage: V8 baseline features enabled, badge-specific additions (NeoPixel, buttons), intentional omissions (HAS_DUAL_BAND)
- [x] I2C bus sharing: touch (FT6336U) and battery (MAX17048) on same SDA/SCL pins (8/9)
- [x] SD card on dedicated SPI bus (pins 35/36/37/47) — no conflict with display SPI (11/12/10)
- [x] Static analysis of config headers — no duplicate or conflicting defines
- [x] Upstream source integration: all Marauder `.cpp` files compile and link against badge config
- [x] `configs.h` shim correctly routes `#include "configs.h"` → `marauder_config.h`
- [x] Include path ordering: `include/` (badge overrides) before `esp32marauder-upstream/esp32_marauder/`

## Needs Hardware Testing ⏳

### PSRAM Detection (Blocker — Issue #29)
- [ ] Run `ESP.getPsramSize()` — record result
- [ ] If PSRAM present, verify speed/mode (QIO vs QSPI)
- [ ] Document actual chip markings on PCB
- [ ] If no PSRAM: reduce buffer sizes in `marauder_config.h` (BUF_SIZE, mac_history_len, MAX_HTML_SIZE)

### Flash Verification
- [ ] Confirm actual flash size: expected 16MB (board JSON says 8MB N8)
- [ ] Verify QIO mode at 80MHz works without corruption

### Display (ILI9341)
- [ ] SPI pins: MOSI(11) SCLK(12) CS(10) DC(5) RST(4) BL(6)
- [ ] Backlight control works (GPIO 6 HIGH = on)
- [ ] Color test pattern renders correctly
- [ ] Landscape rotation (setRotation(1)) correct

### Touch (FT6336U)
- [ ] I2C address detected (SDA:8, SCL:9)
- [ ] Touch coordinates map correctly to display
- [ ] INT pin (7) fires on touch events

### SD Card
- [ ] SPI pins: MOSI(35) SCK(36) MISO(37) CS(47)
- [ ] Card detect and mount
- [ ] Read/write test

### Buttons & Encoder
- [ ] Boot(0), Enter(38), Back(39) — debounce verified
- [ ] Rotary encoder A(45) B(48) Button(20) — direction correct

### NeoPixels & Buzzer
- [ ] 6× WS2812B on GPIO 18
- [ ] Status LED on GPIO 21
- [ ] Buzzer on GPIO 19

### Battery (MAX17048)
- [ ] I2C detection on shared bus (SDA:8, SCL:9)
- [ ] Voltage and SOC readings valid
- [ ] No I2C bus contention with FT6336U touch
