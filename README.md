# BSidesKC Badge WiFi Marauder

ESP32Marauder port for the BSidesKC ESP32-S3 conference badge.

## 🎉 v1.1.0 — Production Ready

Firmware boots and runs on physical BSidesKC badges. All core subsystems verified: display, touch, encoder, WiFi scanning, NeoPixels, buzzer, and badge menu. See the [Hardware Validation Report](HARDWARE_VALIDATION_REPORT.md) for full details.

```
    ┌─────────────────────────────────────────────┐
    │            BSidesKC Badge                    │
    │  ┌───────────────────────────────┐          │
    │  │                               │  [ENC]   │
    │  │      ILI9341 320×240 TFT      │  ◄──►   │
    │  │      FT6336U Touch Layer      │  [BTN]   │
    │  │                               │          │
    │  │    ╔═══════════════════╗      │          │
    │  │    ║  ESP32 Marauder   ║      │          │
    │  │    ║   WiFi │ BLE 5.0  ║      │          │
    │  │    ╚═══════════════════╝      │          │
    │  │                               │          │
    │  └───────────────────────────────┘          │
    │                                              │
    │  ◉◉◉◉◉◉  NeoPixel Ring (×6)    ◉ Status   │
    │                                    LED       │
    │  [BOOT]          [ENTER]       [BACK]       │
    │                                              │
    │  ♪ Buzzer    📡 WiFi/BLE Antenna            │
    │              🔋 MAX17048 Fuel Gauge          │
    │              💾 SD Card Slot                 │
    │                                              │
    │         ESP32-S3 N16R8 (8MB Flash)          │
    │              USB-C (Serial/Power)            │
    └─────────────────────────────────────────────┘
```

## Status

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Project Setup & Build System | ✅ Complete |
| 2 | Display & Input | ✅ Hardware validated |
| 3 | WiFi Features | ✅ Scan validated, attacks compile-verified |
| 4 | BLE Features | ✅ Compile-verified |
| 5 | Storage & Persistence | ⚠️ SD card needs testing |
| 6 | Badge Integration | ✅ Hardware validated |
| 7 | Testing & Polish | ✅ Complete |
| 8 | Hardware Validation | ✅ Core complete |

### Build Stats
- RAM: 21.1% (69 KB / 327 KB)
- Flash: 22.6% (1.48 MB / 6.55 MB)
- Free heap after boot: ~180 KB

## Quick Start — Flashing Your Badge

```bash
# 1. Install PlatformIO
pip install platformio

# 2. Clone and enter repo
git clone https://github.com/BadgePiratesLLC/badgepirates-wifi-marauder.git
cd badgepirates-wifi-marauder
git checkout develop
git submodule update --init

# 3. Connect badge via USB-C

# 4. Build and flash (development)
pio run --target upload

# 5. Monitor serial output (optional)
pio device monitor -b 115200
```

### Production Build

For release firmware with debug logging stripped and optimized flash usage (~9.6KB savings):

```bash
pio run -e bsideskc_prod --target upload
```

The status LED (GPIO 21) blinks 5× on boot to confirm firmware is running. The display shows the splash screen after ~3 seconds, then the Marauder menu.

### Boot Test Modes

Hold these buttons during power-on:

| Button Combo | Test Mode |
|-------------|-----------|
| ENTER | Touch test — draw dots, show coordinates |
| BACK | Encoder test — rotation and button |
| BOOT | Input validation — all inputs |
| ENTER + BACK | WiFi scan test — quick AP scan |

## Hardware

- BSidesKC ESP32-S3 badge (BadgePiratesLLC/QACode_27)
- ILI9341 320×240 TFT + FT6336U capacitive touch
- WiFi + BLE 5.0 (single-band)
- SD card (SPI), NeoPixels (×6 + status), buzzer, rotary encoder
- MAX17048 fuel gauge, 3 buttons (BOOT/ENTER/BACK)
- 8MB Flash, DIO mode, no PSRAM

## Features
- WiFi scanning, deauth, beacon spam, PMKID/EAPOL capture, Evil Portal
- BLE scanning, skimmer detection, BLE spam
- SD card + SPIFFS storage with PCAP saves
- NeoPixel LED feedback, buzzer alerts, battery monitor
- Badge menu system with Marauder launcher
- OTA firmware updates, power management

## Known Issues

| Issue | Severity | Workaround |
|-------|----------|------------|
| SD card not detected on boot | Medium | Use SPIFFS fallback; see [SD troubleshooting](docs/sd-card-troubleshooting.md) |
| Touch debug logging on serial | Low | Cosmetic; remove `Serial.printf` in XPT2046 shim for release |
| 3-second boot delay | Low | By design for USB serial stability |
| No PSRAM | Info | Memory buffers reduced; mac_history=50, HTML=8KB |

## Troubleshooting

### Badge won't boot / no serial output
1. Verify USB-C cable supports data (not charge-only)
2. Hold BOOT button, press RST, release BOOT to enter download mode
3. Flash with `pio run --target upload`
4. Check `pio device monitor -b 115200` for boot log

### Display is blank
- Check that status LED (GPIO 21) blinks on boot — if yes, firmware is running
- Display init happens after 3-second serial delay
- If LED doesn't blink: reflash firmware, try download mode

### Touch not responding / hitting wrong buttons
- Touch coordinates are mapped through XPT2046 shim → FT6336U
- Check serial for `[Touch] raw FT6336U x=... y=...` messages
- If no touch messages: check I2C (SDA:8, SCL:9) connections

### WiFi scan shows no results
- Verify you're in an area with WiFi APs
- Check serial for crash messages during scan
- ESP32-S3 antenna is PCB-integrated; range may be limited

### Boot loop / repeated resets
- Check serial for reset reason: `[BOOT] Reset reason CPU0: X`
  - Reason 1 = normal power-on
  - Reason 3 = software reset (crash)
- If crash: likely memory issue — ensure `HAS_PSRAM` is NOT defined
- Try erasing flash: `pio run --target erase` then reflash

### SD card not working
- Format card as FAT32 (not exFAT)
- Check SPI pins: MOSI:35, SCK:36, MISO:37, CS:47
- Try a different SD card — some cards have SPI compatibility issues

## Documentation

| Document | Description |
|----------|-------------|
| [CHANGELOG.md](CHANGELOG.md) | Release history and version changes |
| [HARDWARE_VALIDATION_REPORT.md](HARDWARE_VALIDATION_REPORT.md) | Boot fixes, serial output, performance metrics |
| [HARDWARE_TESTING_CHECKLIST.md](HARDWARE_TESTING_CHECKLIST.md) | Hardware validation checklist with results |
| [PROJECT_COMPLETION_REPORT.md](PROJECT_COMPLETION_REPORT.md) | Full project completion report |
| [DEPLOYMENT.md](DEPLOYMENT.md) | Build, flash, and deployment instructions |
| [USER_GUIDE.md](USER_GUIDE.md) | End user guide for badge operation |
| [PORTING_PLAN.md](PORTING_PLAN.md) | Full porting plan and phase breakdown |
| [GITHUB_ISSUES.md](GITHUB_ISSUES.md) | Issue tracker reference |
| [SD Card Troubleshooting](docs/sd-card-troubleshooting.md) | SD card debugging and compatibility guide |

### Phase Documentation (in `docs/`)
- [Phase 1](docs/phase1-summary.md) — Build system and project setup
- [Phase 2](docs/phase2-summary.md) — Display and input integration
- [Phase 3](docs/phase3-summary.md) — WiFi feature verification
- [Phase 4](docs/phase4-summary.md) — BLE feature verification
- [Phase 5](docs/phase5-summary.md) — Storage & persistence
- [Phase 6](docs/phase6-summary.md) — Badge integration features
- [Phase 7](docs/phase7-summary.md) — Testing, polish, and final docs

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
