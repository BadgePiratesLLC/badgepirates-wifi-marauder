# BSidesKC Badge WiFi Marauder

ESP32Marauder port for the BSidesKC ESP32-S3 conference badge.

## Status: Phase 4 Complete

Phase 4 (BLE Features) is complete — all upstream BLE features compile successfully for ESP32-S3 using NimBLE 1.4.3. Hardware runtime testing deferred to when badges are available.

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Project Setup & Build System | ✅ Complete |
| 2 | Display & Input | ✅ Complete |
| 3 | WiFi Features | ✅ Complete (compile-verified) |
| 4 | BLE Features | ✅ Complete (compile-verified) |
| 5 | Storage & Persistence | 🔄 Next |
| 6 | Badge Integration | ⏳ Pending |
| 7 | Testing & Polish | ⏳ Pending |

### Phase 4 Highlights
- NimBLE 1.4.3 fully compatible with ESP32-S3 (no upgrade to 2.x needed)
- ESP32-S3 BLE 5.0 support confirmed (2M PHY, Coded PHY, extended advertising available)
- All BLE features compile: scanning, skimmer detect, AirTag scan/spoof, BLE spam (Apple/Samsung/Google/Windows/Flipper)
- Zero upstream modifications maintained
- Build: 21.0% RAM, 22.4% Flash

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
- [Testing Guide](docs/testing-guide.md) — Test procedures for all phases
- [WiFi Implementation Notes](docs/wifi-implementation-notes.md) — ESP32-S3 WiFi analysis
- [BLE Implementation Notes](docs/ble-implementation-notes.md) — ESP32-S3 BLE/NimBLE analysis

## Based On
- [ESP32Marauder](https://github.com/justcallmekoko/ESP32Marauder) by justcallmekoko
- [QACode_27](https://github.com/BadgePiratesLLC/QACode_27) badge firmware by BadgePiratesLLC

## License
See upstream ESP32Marauder license.
