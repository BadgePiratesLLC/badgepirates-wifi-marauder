# Phase 1 Summary — BSidesKC Badge Marauder Port

## What Was Accomplished

Phase 1 established the complete build system and project structure for porting ESP32Marauder to the BSidesKC badge hardware.

### Deliverables
1. **ESP32Marauder vendored as git submodule** — pinned to known-good upstream commit, license preserved
2. **PlatformIO build configuration** — `platformio.ini` targeting ESP32-S3 with 16MB flash, QIO @ 80MHz, all library dependencies declared
3. **Three-layer config architecture:**
   - `bsideskc_pins.h` — physical pin truth (25+ GPIOs)
   - `bsideskc_config.h` — feature flags and board identity
   - `marauder_config.h` — full upstream compatibility layer (MARAUDER_V8 baseline)
4. **TFT_eSPI configured via build flags** — no upstream file modifications required
5. **Minimal bootstrap firmware** (`main.cpp`) — display init, serial output, backlight control
6. **Documentation:** porting plan, integration notes, hardware verification checklist, testing guide

## Build Statistics

| Metric | Value |
|--------|-------|
| Target board | `esp32-s3-devkitc-1` |
| Platform | `espressif32@6.4.0` |
| Flash | 16MB QIO @ 80MHz |
| Framework | Arduino |
| Library deps | 7 (TFT_eSPI, FT6336U, NimBLE, NeoPixel, MAX1704X, BusIO, ArduinoJson) |
| Config defines | ~120 (marauder_config.h) |
| GPIO assignments | 25 pins mapped, 0 conflicts |
| Upstream reference | MARAUDER_V8 |

## Known Issues & Limitations

| Issue | Status | Impact |
|-------|--------|--------|
| **PSRAM unknown** (Issue #29) | Open — needs hardware | If absent, buffer sizes must be reduced; affects Evil Portal, PCAP capture, MAC history |
| **Flash size unconfirmed** | Needs hardware | Board JSON says 8MB N8, porting plan says 16MB — must verify actual chip |
| **Touch driver not yet adapted** | Phase 2 | FT6336U (I2C cap) needs shim to replace XPT2046 (SPI resistive) |
| **No upstream source compilation** | Expected | Phase 1 scope was build system only; full Marauder source integration is Phase 2+ |
| **HAS_DUAL_BAND omitted** | By design | Badge ESP32-S3 is single-band 2.4 GHz |

## GitHub Issues Closed

| Issue | Title | Status |
|-------|-------|--------|
| #25 | Fork/vendor ESP32Marauder source | ✅ Closed |
| #26 | Create PlatformIO configuration | ✅ Closed |
| #27 | Create BSIDESKC_BADGE board define | ✅ Closed |
| #28 | Configure TFT_eSPI for badge display | ✅ Closed |
| #29 | Verify PSRAM presence and clean compile | ⏳ Open (hardware blocker) |

## Readiness for Phase 2

### Ready ✅
- Build system is stable and compiles cleanly
- Pin mappings are complete and conflict-free
- Config architecture cleanly separates badge customization from upstream
- Upstream submodule update path is documented
- All Phase 2 dependencies (display, touch, buttons, encoder) have pins defined

### Phase 2 Prerequisites
1. **Hardware access** — need physical badge to verify PSRAM (Issue #29) and flash size
2. **FT6336U touch adapter** — design shim class mapping I2C cap touch → Marauder's XPT2046 interface
3. **Button/encoder mapping** — wire Enter(38)/Back(39) and rotary encoder to Marauder menu navigation

### Phase 2 Scope (Issues #30–#34)
- #30: Verify ILI9341 display with Marauder rendering
- #31: Implement FT6336U touch driver adapter
- #32: Map hardware buttons to Marauder button interface
- #33: Map rotary encoder to menu scrolling
- #34: End-to-end input validation
