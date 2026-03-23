# BSidesKC Badge WiFi Marauder

ESP32Marauder port for the BSidesKC ESP32-S3 conference badge.

## Status: Phase 6 Complete

Phase 6 (Badge Integration) is complete — all badge-specific hardware features compile successfully for ESP32-S3. NeoPixel LED feedback, buzzer tones, battery monitoring, menu integration, OTA update support, and power management all verified at compile time. Hardware runtime testing deferred to when badges are available.

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Project Setup & Build System | ✅ Complete |
| 2 | Display & Input | ✅ Complete |
| 3 | WiFi Features | ✅ Complete (compile-verified) |
| 4 | BLE Features | ✅ Complete (compile-verified) |
| 5 | Storage & Persistence | ✅ Complete (compile-verified) |
| 6 | Badge Integration | ✅ Complete (compile-verified) |
| 7 | Testing & Polish | 🔄 Next |

### Phase 6 Highlights
- NeoPixel LED feedback: 5 patterns (idle, scanning, attack, capture, error) on 6× ring + status LED
- Buzzer audio: 6 tone patterns for menu, scan, capture, error, low battery events
- Battery monitoring: MAX17048 fuel gauge with low (15%) and critical (5%) warnings
- Badge menu: LED brightness, buzzer mute, battery status, hardware test submenu
- OTA update: Upstream Marauder web update via WiFi AP
- Power management: Auto-dim (2min), auto-sleep (5min), wake-on-button
- Build: 21.1% RAM, 22.6% Flash

## Hardware
- BSidesKC ESP32-S3 badge (BadgePiratesLLC/QACode_27)
- ILI9341 320×240 TFT + FT6336U capacitive touch
- WiFi + BLE 5.0
- SD card, NeoPixels, buzzer, rotary encoder

## Building

Prerequisites: [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html)

```bash
# Build firmware
pio run

# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

## Documentation

See the [`docs/`](docs/) directory for detailed documentation:
- [PORTING_PLAN.md](PORTING_PLAN.md) — Full porting plan and phase breakdown
- [GITHUB_ISSUES.md](GITHUB_ISSUES.md) — Issue tracker reference
- [Phase 1 Summary](docs/phase1-summary.md) — Build system and project setup
- [Phase 2 Summary](docs/phase2-summary.md) — Display and input integration
- [Phase 3 Summary](docs/phase3-summary.md) — WiFi feature verification
- [Phase 4 Summary](docs/phase4-summary.md) — BLE feature verification
- [Phase 5 Summary](docs/phase5-summary.md) — Storage & persistence verification
- [Phase 6 Summary](docs/phase6-summary.md) — Badge integration features
- [Testing Guide](docs/testing-guide.md) — Test procedures for all phases
- [WiFi Implementation Notes](docs/wifi-implementation-notes.md) — ESP32-S3 WiFi analysis
- [BLE Implementation Notes](docs/ble-implementation-notes.md) — ESP32-S3 BLE/NimBLE analysis

## Based On
- [ESP32Marauder](https://github.com/justcallmekoko/ESP32Marauder) by justcallmekoko
- [QACode_27](https://github.com/BadgePiratesLLC/QACode_27) badge firmware by BadgePiratesLLC

## License
See upstream ESP32Marauder license.
