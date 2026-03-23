# BSidesKC Badge WiFi Marauder

ESP32Marauder port for the BSidesKC ESP32-S3 conference badge.

## Status: Phase 3 Up Next

Phases 1 & 2 complete. WiFi features (Phase 3) starting next. Track progress via [GitHub Issues](https://github.com/BadgePiratesLLC/badgepirates-wifi-marauder/issues).

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Project Setup & Build System | ✅ Complete |
| 2 | Display & Input | ✅ Complete |
| 3 | WiFi Features (Issues #35-41) | 🔄 Up Next |
| 4 | BLE Features | ⏳ Pending |
| 5 | Storage & Persistence | ⏳ Pending |
| 6 | Badge Integration | ⏳ Pending |
| 7 | Testing & Polish | ⏳ Pending |

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

## Based On
- [ESP32Marauder](https://github.com/justcallmekoko/ESP32Marauder) by justcallmekoko
- [QACode_27](https://github.com/BadgePiratesLLC/QACode_27) badge firmware by BadgePiratesLLC

## License
See upstream ESP32Marauder license.
