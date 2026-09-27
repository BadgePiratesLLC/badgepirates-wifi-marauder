# BSidesKC Badge WiFi Marauder

ESP32Marauder port for the BadgePirates ESP32-S3 conference badges.

## Badge / Environment Matrix

One firmware, one repo, per-board config headers behind a `BADGE_HW_*` build flag (Nexus b5a17dba) — no board is "the default" env anymore.

| Badge | PlatformIO env | What's different | Verified date | Verified by |
|-------|-----------------|-------------------|----------------|-------------|
| CC14 / BSidesKC26 | `bsideskc-badge-cc14` (alias: `bsideskc-badge`, kept so existing scripts/docs don't break) | Baseline — panel front-mounted, `SCREEN_ORIENTATION 1`, un-mirrored FT6336U touch mapping | Build verified on localllm 2026-09-26 (`pio run -e bsideskc-badge-cc14` succeeds). **Not yet flashed to a physical CC14 board or hardware-checked by Kevin** — 2026-09-24 only produced a verbal pinout-equivalence remark ("14 and 15 have the same screen and pinout"), not a flash/screen/touch check. | Build: agent. Hardware: unverified — pending Kevin. |
| CC13 / BSidesKC25 | `bsideskc-badge-cc13` | `BADGE_HW_CC13`: panel back-mounted, rotated 180° (`SCREEN_ORIENTATION 3`), FT6336U touch endpoints reversed to match. Pins identical to CC14 (Nexus 2977d950). | 2026-09-24 (GAMINGCRAP COM3) | Kevin |
| CC15 | not yet added — **do not add as a flag.** Bucky pulled the actual CC15 KiCad source (2026-09-26): screen header J3 is **DNP** (not populated), there is **no FT6336U/touch-controller symbol** anywhere in the schematic, and `RotaryEncoders.kicad_sch` is an **empty, unwired stub**. This contradicts the earlier verbal "14 and 15 are identical" note and is a bigger gap than an orientation delta — CC15's display/touch/encoder hardware isn't finalized/populated on the boards Bucky checked. Per this ticket's own escape hatch, that's a separate hardware-bringup ticket with Bucky, not a `BADGE_HW_CC15` flag bodged in here. | — | — |

**CC13 / BSidesKC25 display — orientation fix (Nexus 2977d950):** On these two badge years, the screen was mounted on the *back* of the board instead of the front, so the panel is physically rotated 180° relative to CC14/BSidesKC26. Confirmed against the CC13 schematic (`BadgePiratesLLC/Project-CC13`, `CAD/Screens.kicad_sch`) that this is orientation-only — all 11 display/touch GPIOs match `include/bsideskc_pins.h` exactly, no pin remap. The fix is a `BADGE_HW_CC13` build flag (`env:bsideskc-badge-cc13` PlatformIO environment) that flips `SCREEN_ORIENTATION` to TFT_eSPI rotation 3 (180°-flipped landscape) and mirrors the FT6336U touch-coordinate mapping in `include/XPT2046_Touchscreen.h` to match. CC14/BSidesKC26 (`env:bsideskc-badge-cc14`, alias `env:bsideskc-badge`) is untouched. **Verified on physical hardware by Kevin, 2026-09-24** (CC13 on GAMINGCRAP COM3): display renders right side up, touch lands where pressed.

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
bash patches/apply.sh  # small compile-compat patches upstream can't take (see patches/apply.sh)

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
This is a port of [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder)
by [justcallmekoko](https://github.com/justcallmekoko) to the BSidesKC
ESP32-S3 badge, built on top of
[QACode_27](https://github.com/BadgePiratesLLC/QACode_27) badge firmware by
BadgePiratesLLC. The `esp32marauder-upstream` git submodule pulls in the
upstream project's source directly; this repo's own code is the badge
integration layer (display/touch/menu/hardware glue) around it.

## License
MIT — see [LICENSE](LICENSE). Same license as upstream ESP32 Marauder.

This build also links two LGPL-3.0 libraries (ESPAsyncWebServer, AsyncTCP)
for the Evil Portal feature — see [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
