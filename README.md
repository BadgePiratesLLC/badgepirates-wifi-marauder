# BSidesKC Badge WiFi Marauder

ESP32Marauder port for the BSidesKC ESP32-S3 conference badge.

## 🎉 Project Complete — v1.0.0-simulation-complete

All 7 development phases are finished. The firmware compiles cleanly for the BSidesKC ESP32-S3 badge with all Marauder features ported. **Hardware runtime testing is pending** — awaiting physical badges.

📄 **[Full Completion Report →](PROJECT_COMPLETION_REPORT.md)**  
🔧 **[Hardware Testing Checklist →](HARDWARE_TESTING_CHECKLIST.md)**

## Status

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Project Setup & Build System | ✅ Complete |
| 2 | Display & Input | ✅ Complete |
| 3 | WiFi Features | ✅ Compile-verified |
| 4 | BLE Features | ✅ Compile-verified |
| 5 | Storage & Persistence | ✅ Compile-verified |
| 6 | Badge Integration | ✅ Compile-verified |
| 7 | Testing & Polish | ✅ Complete |

### Build Stats
- RAM: 21.1% (69 KB / 327 KB)
- Flash: 22.6% (1.48 MB / 6.55 MB)

### Hardware Testing Status
⏳ **Awaiting physical badges.** All features are compile-verified and code-complete. See [HARDWARE_TESTING_CHECKLIST.md](HARDWARE_TESTING_CHECKLIST.md) for the full validation plan.

## Hardware
- BSidesKC ESP32-S3 badge (BadgePiratesLLC/QACode_27)
- ILI9341 320×240 TFT + FT6336U capacitive touch
- WiFi + BLE 5.0
- SD card, NeoPixels, buzzer, rotary encoder

## Features
- WiFi scanning, deauth, beacon spam, PMKID/EAPOL capture, Evil Portal
- BLE scanning, skimmer detection, BLE spam
- SD card + SPIFFS storage with PCAP saves
- NeoPixel LED feedback, buzzer alerts, battery monitor
- Badge menu system with Marauder launcher
- OTA firmware updates, power management

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

| Document | Description |
|----------|-------------|
| [PROJECT_COMPLETION_REPORT.md](PROJECT_COMPLETION_REPORT.md) | Full project completion report |
| [HARDWARE_TESTING_CHECKLIST.md](HARDWARE_TESTING_CHECKLIST.md) | Hardware validation checklist |
| [DEPLOYMENT.md](DEPLOYMENT.md) | Build, flash, and deployment instructions |
| [USER_GUIDE.md](USER_GUIDE.md) | End user guide for badge operation |
| [PORTING_PLAN.md](PORTING_PLAN.md) | Full porting plan and phase breakdown |
| [GITHUB_ISSUES.md](GITHUB_ISSUES.md) | Issue tracker reference |

### Phase Documentation (in `docs/`)
- [Phase 1](docs/phase1-summary.md) — Build system and project setup
- [Phase 2](docs/phase2-summary.md) — Display and input integration
- [Phase 3](docs/phase3-summary.md) — WiFi feature verification
- [Phase 4](docs/phase4-summary.md) — BLE feature verification
- [Phase 5](docs/phase5-summary.md) — Storage & persistence
- [Phase 6](docs/phase6-summary.md) — Badge integration features
- [Phase 7](docs/phase7-summary.md) — Testing, polish, and final docs
- [Testing Guide](docs/testing-guide.md) — Test procedures for all phases

## Contributing

1. Fork the repository
2. Create a feature branch from `develop` (`git checkout -b feature/your-feature develop`)
3. Make your changes with clear commit messages
4. Test that the build compiles: `pio run`
5. Submit a pull request to `develop`

### Guidelines
- Follow existing code style and file organization
- Add documentation for new features in `docs/`
- Update pin mappings in `include/bsideskc_pins.h` if adding hardware
- Test compile before submitting — `pio run` must succeed
- Hardware-dependent changes should include test procedures

## Based On
- [ESP32Marauder](https://github.com/justcallmekoko/ESP32Marauder) by justcallmekoko
- [QACode_27](https://github.com/BadgePiratesLLC/QACode_27) badge firmware by BadgePiratesLLC

## License
See upstream ESP32Marauder license.
