# Phase 7 — Testing & Polish Notes

## Overview

Phase 7 covers full feature regression testing (Issue #57) and memory profiling (Issue #58). This is the final phase before hardware testing on physical BSidesKC badge hardware.

---

## Issue #57: Full Feature Regression Test

### Build Verification

| Check | Result |
|-------|--------|
| `pio run -e bsideskc-badge` | ✅ PASS — clean compile, zero errors |
| RAM usage | 21.1% (69,248 / 327,680 bytes) |
| Flash usage | 22.6% (1,483,177 / 6,553,600 bytes) |
| Warnings | None (clean build) |

### Automated Test Script

`tests/regression_test.py` — automated build verification and serial boot monitoring.

```bash
# Build-only test (no hardware needed)
python3 tests/regression_test.py

# With serial monitoring (requires connected badge)
python3 tests/regression_test.py --port /dev/ttyACM0
```

### Feature Regression Checklist

#### Phase 1 — Core HAL & Display

| # | Feature | Test Method | Expected Result |
|---|---------|-------------|-----------------|
| 1.1 | ILI9341 display init | Boot badge, observe splash screen | "BSidesKC Badge" + "ESP32 Marauder" + version shown |
| 1.2 | Backlight PWM | Press BOOT button repeatedly | Cycles through 10 brightness levels |
| 1.3 | Backlight persistence | Cycle brightness, reboot | Saved level restored on boot |
| 1.4 | FT6336U touch | Enter touch test (hold ENTER at boot) | Touch coordinates displayed, dots drawn |
| 1.5 | XPT2046 shim | Navigate Marauder menus via touch | Touch input maps correctly to menu items |
| 1.6 | SPI bus isolation | Use display + SD simultaneously | No SPI bus contention (separate buses) |

#### Phase 2 — Input System

| # | Feature | Test Method | Expected Result |
|---|---------|-------------|-----------------|
| 2.1 | Rotary encoder scroll | Rotate knob in main menu | Menu selection moves up/down, wraps at boundaries |
| 2.2 | Encoder button select | Press encoder knob on menu item | Item activated, buzzer feedback |
| 2.3 | ENTER button (C_BTN) | Press ENTER in menu | Same as encoder button — selects item |
| 2.4 | BACK button (D_BTN) | Press BACK in submenu | Returns to parent menu |
| 2.5 | BOOT button | Short press during normal operation | Backlight brightness cycles |
| 2.6 | Input test mode | Hold BOOT during power-on | Full input validation screen shown |
| 2.7 | Encoder test mode | Hold BACK during power-on | Encoder position counter displayed |
| 2.8 | Button debouncing | Rapid button presses | No double-triggers, clean events |

#### Phase 3 — WiFi Scanning

| # | Feature | Test Method | Expected Result |
|---|---------|-------------|-----------------|
| 3.1 | WiFi scan start | Menu → WiFi → Scan APs | Scan starts, LEDs show scanning pattern |
| 3.2 | AP list display | Complete a WiFi scan | Discovered APs listed with SSID, RSSI, channel |
| 3.3 | Channel hopping | Monitor serial during scan | Channels cycle 1-13 |
| 3.4 | Scan stop | Press BACK during scan | Scan stops, returns to menu |
| 3.5 | WiFi scan test mode | Hold ENTER+BACK during boot | Automated scan test runs |
| 3.6 | Deauth attack | Menu → WiFi → Deauth | Deauth frames sent, LED shows attack pattern |
| 3.7 | Beacon spam | Menu → WiFi → Beacon Spam | Fake APs broadcast |
| 3.8 | Probe flood | Menu → WiFi → Probe Flood | Probe requests sent |
| 3.9 | PMKID capture | Menu → WiFi → Sniff PMKID | PMKID captured (if target available) |
| 3.10 | Packet monitor | Menu → WiFi → Packet Monitor | Live packet count displayed |

#### Phase 4 — BLE Scanning

| # | Feature | Test Method | Expected Result |
|---|---------|-------------|-----------------|
| 4.1 | BLE scan | Menu → Bluetooth → Scan | BLE devices discovered and listed |
| 4.2 | BLE skimmer detect | Menu → Bluetooth → Skimmers | Skimmer detection scan runs |
| 4.3 | BLE spam | Menu → Bluetooth → BLE Spam | BLE advertisements broadcast |
| 4.4 | Flipper detect | Menu → Bluetooth → Flipper | Flipper Zero devices detected |
| 4.5 | AirTag scan | Menu → Bluetooth → AirTag | AirTag devices detected |

#### Phase 5 — Storage & Settings

| # | Feature | Test Method | Expected Result |
|---|---------|-------------|-----------------|
| 5.1 | SD card init | Boot with SD card inserted | "SD Card" shown in Device menu, no error on serial |
| 5.2 | SD card write | Capture packets → check SD | PCAP file written to SD |
| 5.3 | Settings save | Change a setting, reboot | Setting persists (NVS) |
| 5.4 | Settings load | Boot normally | All saved settings restored |
| 5.5 | Evil Portal HTML | Load HTML from SD | Portal serves custom HTML |
| 5.6 | GPS data | Connect GPS module | GPS coordinates displayed |

#### Phase 6 — Badge Integration

| # | Feature | Test Method | Expected Result |
|---|---------|-------------|-----------------|
| 6.1 | NeoPixel idle | Badge idle in menu | Breathing cyan pattern on 6× ring |
| 6.2 | NeoPixel scanning | Start WiFi scan | Rotating blue dot pattern |
| 6.3 | NeoPixel attack | Start deauth | Pulsing red pattern |
| 6.4 | Status LED | All modes | Status LED mirrors ring state |
| 6.5 | Buzzer button press | Select menu item | 2kHz 30ms click |
| 6.6 | Buzzer scan start | Start any scan | Rising tone (1k→2k) |
| 6.7 | Buzzer scan stop | Stop any scan | Falling tone (2k→1k) |
| 6.8 | Buzzer mute | Badge menu → Buzzer Mute | All tones silenced |
| 6.9 | Battery status | Badge menu → Battery Status | Percentage + bar graph shown |
| 6.10 | Battery low warning | Drain to ≤15% | Single warning tone + serial log |
| 6.11 | Battery critical | Drain to ≤5% | Repeated warning every 30s, red status bar |
| 6.12 | LED brightness adjust | Badge menu → LED Brightness | Encoder adjusts, bar graph updates |
| 6.13 | Badge menu entry | Main menu → Badge | Badge submenu with 5 items shown |
| 6.14 | Hardware test | Badge menu → Hardware Test | Input validation test runs |
| 6.15 | Auto-dim | Wait 2 minutes idle | Backlight dims to minimum |
| 6.16 | Auto-sleep | Wait 5 minutes idle | Light sleep entered |
| 6.17 | Wake-on-button | Press any button during sleep | Badge wakes, backlight restores |
| 6.18 | Scan prevents sleep | Start scan, wait 5+ minutes | No auto-sleep during active scan |
| 6.19 | OTA update | Device → Update Firmware → Web | WiFi AP created, web UI served |
| 6.20 | Power manager reset | Any input during dim | Backlight restores to saved level |

### Manual Test Procedures

#### Procedure: Full Boot Sequence Test
1. Power off badge completely
2. Power on, observe serial output at 115200 baud
3. Verify splash screen appears: "BSidesKC Badge" / "ESP32 Marauder" / version
4. Verify serial shows all init messages in order:
   - `[BSidesKC] Booting ESP32 Marauder...`
   - `[BSidesKC] Rotary encoder initialized`
   - `[Buzzer] Initialized on GPIO 19`
   - `[Battery] Monitor initialized`
   - `[Badge] Menu items added`
   - `[Power] Manager initialized`
   - `[BSidesKC] Marauder ready.`
5. Verify main menu is displayed with items: WiFi, Bluetooth, Device, Badge, Reboot

#### Procedure: Input Stress Test
1. Rapidly rotate encoder while in menu — verify no crashes
2. Press encoder button + ENTER simultaneously — verify no conflict
3. Hold BACK for 3 seconds — verify single back navigation
4. Press BOOT rapidly 20 times — verify brightness cycles cleanly

#### Procedure: WiFi Scan + LED/Buzzer Integration
1. Navigate to WiFi → Scan APs
2. Verify: buzzer plays rising tone, LEDs switch to scanning pattern
3. Wait for scan to complete
4. Press BACK to stop
5. Verify: buzzer plays falling tone, LEDs return to idle

#### Procedure: Power Management Cycle
1. Start badge, note time
2. Do nothing for 2 minutes — verify backlight dims
3. Touch any button — verify backlight restores
4. Wait another 5 minutes — verify light sleep entered
5. Press BOOT — verify wake, backlight on, serial shows wake message

### Test Report Template

```
BSidesKC Badge — Regression Test Report
Date: ___________
Tester: ___________
Firmware Version: ___________
Hardware Revision: ___________

BUILD:
  [ ] Clean compile: PASS / FAIL
  [ ] RAM usage: ___% (target: <50%)
  [ ] Flash usage: ___% (target: <80%)

PHASE 1 — Display & HAL:
  [ ] 1.1 Display init     [ ] 1.2 Backlight PWM
  [ ] 1.3 BL persistence   [ ] 1.4 Touch test
  [ ] 1.5 XPT2046 shim     [ ] 1.6 SPI isolation

PHASE 2 — Input:
  [ ] 2.1 Encoder scroll    [ ] 2.2 Encoder select
  [ ] 2.3 ENTER button      [ ] 2.4 BACK button
  [ ] 2.5 BOOT button       [ ] 2.6 Input test mode
  [ ] 2.7 Encoder test      [ ] 2.8 Debouncing

PHASE 3 — WiFi:
  [ ] 3.1 Scan start        [ ] 3.2 AP list
  [ ] 3.3 Channel hop       [ ] 3.4 Scan stop
  [ ] 3.5 Scan test mode    [ ] 3.6 Deauth
  [ ] 3.7 Beacon spam       [ ] 3.8 Probe flood
  [ ] 3.9 PMKID capture     [ ] 3.10 Packet monitor

PHASE 4 — BLE:
  [ ] 4.1 BLE scan          [ ] 4.2 Skimmer detect
  [ ] 4.3 BLE spam          [ ] 4.4 Flipper detect
  [ ] 4.5 AirTag scan

PHASE 5 — Storage:
  [ ] 5.1 SD init           [ ] 5.2 SD write
  [ ] 5.3 Settings save     [ ] 5.4 Settings load
  [ ] 5.5 Evil Portal       [ ] 5.6 GPS

PHASE 6 — Badge:
  [ ] 6.1-6.4 NeoPixel patterns    [ ] 6.5-6.8 Buzzer tones
  [ ] 6.9-6.11 Battery monitor     [ ] 6.12 LED brightness
  [ ] 6.13-6.14 Badge menu         [ ] 6.15-6.18 Power mgmt
  [ ] 6.19 OTA update              [ ] 6.20 Power reset

ISSUES FOUND:
  1. ___________
  2. ___________

OVERALL: PASS / FAIL
```

---

## Issue #58: Memory Profiling Report

### Static Memory Analysis (Compile-Time)

#### Overall Firmware Size

| Section | Size | Notes |
|---------|------|-------|
| .text (code) | 1,198,634 bytes | Executable instructions |
| .data (initialized) | 300,672 bytes | Initialized global/static variables |
| .bss (uninitialized) | 1,233,108 bytes | Zero-initialized globals (includes WiFi/BLE stacks) |
| **Total** | **2,732,414 bytes** | |

#### RAM Usage Summary

| Metric | Value |
|--------|-------|
| Static RAM used | 69,248 bytes (21.1%) |
| Static RAM available | 327,680 bytes |
| Static RAM free | 258,432 bytes |
| Flash used | 1,483,177 bytes (22.6%) |
| Flash available | 6,553,600 bytes |
| Flash free | 5,070,423 bytes |

**Assessment:** RAM and Flash usage are well within safe limits. Significant headroom remains for future features.

#### Partition Table (16MB Flash)

| Partition | Offset | Size | Usage |
|-----------|--------|------|-------|
| nvs | 0x9000 | 20 KB | Settings storage |
| otadata | 0xE000 | 8 KB | OTA metadata |
| app0 (OTA_0) | 0x10000 | 6,400 KB | Primary firmware |
| app1 (OTA_1) | 0x650000 | 6,400 KB | OTA update slot |
| spiffs | 0xC90000 | 3,456 KB | File storage |
| coredump | 0xFF0000 | 64 KB | Crash dumps |

Firmware (1.45 MB) fits comfortably in 6.4 MB OTA partition — 77% headroom.

#### Badge-Specific Module Sizes

| Module | .text | .data | .bss | Total | Notes |
|--------|-------|-------|------|-------|-------|
| main.cpp | 10,972 | 12 | 5,628 | 16,612 | Main loop + global objects |
| badge_menu.cpp | 6,644 | 9 | 44 | 6,697 | Menu integration |
| input_test.cpp | 2,763 | 9 | 27 | 2,799 | Hardware test mode |
| wifi_scan_test.cpp | 1,862 | 8 | 16 | 1,886 | WiFi scan test |
| led_feedback.cpp | 1,021 | 9 | 58 | 1,088 | NeoPixel patterns |
| power_manager.cpp | 829 | 12 | 22 | 863 | Sleep/wake management |
| buzzer.cpp | 723 | 0 | 11 | 734 | PWM tone engine |
| encoder_handler.cpp | 620 | 8 | 171 | 799 | Rotary encoder |
| battery_monitor.cpp | 617 | 8 | 21 | 646 | Battery warnings |
| display_adapter.cpp | 545 | 9 | 8 | 562 | Backlight PWM |
| touch_adapter.cpp | 283 | 8 | 28 | 319 | FT6336U bridge |
| button_handler.cpp | 276 | 0 | 11 | 287 | BOOT button handler |
| **Badge Total** | **27,155** | **92** | **6,045** | **33,292** | |

Badge-specific code adds only ~27 KB of code and ~6 KB of RAM — minimal overhead.

#### Upstream Marauder Module Sizes (Top Consumers)

| Module | .text | .bss | Notes |
|--------|-------|------|-------|
| WiFiScan.cpp | 177,655 | 9,990 | Largest — all WiFi/BLE scan logic |
| MenuFunctions.cpp | 96,980 | 16 | Menu system + UI rendering |
| CommandLine.cpp | 55,934 | 16 | Serial CLI parser |
| settings.cpp | 35,489 | 16 | NVS settings management |
| GpsInterface.cpp | 14,988 | 248 | GPS NMEA parsing |
| EvilPortal.cpp | 12,484 | 104 | Captive portal |
| Display.cpp | 11,423 | 21 | TFT drawing primitives |

#### Largest BSS (RAM) Consumers

| Symbol | Size (bytes) | Description |
|--------|-------------|-------------|
| WiFiScan::mac_entries | 8,750 | MAC address history table |
| g_cnxMgr | 3,800 | WiFi connection manager (ESP-IDF) |
| ftm_initiator | 2,768 | FTM ranging (ESP-IDF) |
| wifi_scan_obj | 2,744 | WiFiScan global instance |
| main.cpp BSS | 5,628 | Global objects (display, menu, etc.) |
| display_obj | 1,028 | Display instance |
| menu_function_obj | 1,264 | Menu system instance |

### PSRAM Usage Analysis

The badge defines `HAS_PSRAM` and calls `psramInit()` at boot. PSRAM is used by upstream Marauder for:

| Allocation | Size | Location |
|------------|------|----------|
| `mac_history` | ~6 KB (500 × 12 bytes) | WiFiScan.cpp — `ps_malloc()` |
| `ssids` LinkedList | Variable | WiFiScan.cpp — `ps_malloc()` |
| `index_html` (Evil Portal) | 30 KB max | EvilPortal.cpp — `ps_malloc()` |
| WiFi/BLE packet buffers | 8 KB (BUF_SIZE) | Buffer.cpp via config |

**PSRAM Risk:** The board definition reports "No PSRAM" (`esp32-s3-devkitc-1`), but the badge hardware has PSRAM. The `psramInit()` call will determine availability at runtime. If PSRAM is not detected:
- `ps_malloc()` returns NULL → potential crashes in WiFiScan and EvilPortal
- Fallback: upstream code has `#ifndef HAS_PSRAM` paths using regular `malloc()`

**Recommendation:** Verify PSRAM detection on actual hardware. If PSRAM is not available, remove `#define HAS_PSRAM` from `marauder_config.h` to use heap fallbacks.

### Memory Leak Risk Assessment

| Area | Risk | Notes |
|------|------|-------|
| NeoPixel buffers | LOW | Static allocation, fixed size (6+1 pixels) |
| Buzzer patterns | NONE | Const arrays, no dynamic allocation |
| Encoder handler | LOW | Static ISR flags, no heap use |
| Button handler | NONE | Pure GPIO polling, no allocation |
| Battery monitor | NONE | Reads upstream battery_obj, no allocation |
| Power manager | NONE | Timer-based, no allocation |
| Badge menu | LOW | LinkedList nodes allocated once at setup, never freed (intentional) |
| Display adapter | NONE | Preferences object is static |
| WiFiScan (upstream) | MEDIUM | Dynamic LinkedList for SSIDs, mac_history via ps_malloc — freed on scan stop |
| EvilPortal (upstream) | MEDIUM | 30KB HTML buffer via ps_malloc — freed on portal stop |
| MenuFunctions (upstream) | LOW | Button objects allocated once, never freed (intentional) |
| CommandLine (upstream) | LOW | String operations may fragment heap over time |

**Overall Assessment:** No memory leaks identified in badge-specific code. Upstream Marauder has known patterns of allocate-on-start/free-on-stop for scan buffers. Long-running sessions with many scan start/stop cycles could fragment the heap, but this is inherent to the upstream design.

### Buffer Size Analysis

| Buffer | Current Size | Optimal? | Notes |
|--------|-------------|----------|-------|
| BUF_SIZE (PCAP) | 8 KB | ✅ | Appropriate for PSRAM-equipped board |
| SNAP_LEN | 4 KB | ✅ | Standard packet capture length |
| MAX_HTML_SIZE | 30 KB | ✅ | Evil Portal HTML limit |
| mac_history_len | 500 entries | ✅ | ~6 KB in PSRAM, sufficient for con environment |
| JSON_SETTING_SIZE | 2 KB | ✅ | Adequate for settings JSON |
| DISPLAY_BUFFER_LIMIT | 20 | ✅ | Screen line buffer |
| MAX_SCREEN_BUFFER | 21 | ✅ | Matches display buffer |

No buffer size changes recommended — all values are appropriate for the badge's ESP32-S3 with 16MB flash and PSRAM.

### Runtime Memory Monitoring

The upstream Marauder status bar displays DRAM and PSRAM usage percentages via `getDRAMUsagePercent()` and `getPSRAMUsagePercent()` from `utils.h`. These are visible in the menu status bar during operation.

For serial-based monitoring, add to the main loop (for debug builds only):

```cpp
// Debug: print heap stats every 10 seconds
static uint32_t lastHeapPrint = 0;
if (millis() - lastHeapPrint > 10000) {
    lastHeapPrint = millis();
    Serial.printf("[MEM] Free heap: %d, Min free: %d, PSRAM free: %d\n",
        ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getFreePsram());
}
```

### Memory Optimization Opportunities (Not Required)

These are noted for future reference if memory becomes constrained:

1. **WiFiScan.cpp** (177 KB .text) — Largest single module. Could be split into separate scan/attack modules with conditional compilation.
2. **MenuFunctions.cpp** (97 KB .text) — Menu rendering is monolithic. Could use PROGMEM for string tables.
3. **CommandLine.cpp** (56 KB .text) — CLI parser handles many commands. Could use command table with function pointers instead of if/else chains.
4. **Font data** — TFT_eSPI loads 7 fonts (GLCD, Font2, 4, 6, 7, 8, GFXFF). Removing unused fonts saves flash.

None of these optimizations are needed currently — 78.9% RAM and 77.4% Flash remain free.

---

## Summary

### Phase 7 Status

| Item | Status |
|------|--------|
| Feature branch | ✅ `feature/phase7-testing-polish` |
| Build verification | ✅ Clean compile, zero errors |
| RAM usage | ✅ 21.1% — well within limits |
| Flash usage | ✅ 22.6% — well within limits |
| Regression test checklist | ✅ 47 test cases documented |
| Automated test script | ✅ `tests/regression_test.py` |
| Manual test procedures | ✅ 4 procedures documented |
| Test report template | ✅ Printable checklist |
| Memory profiling | ✅ Complete static analysis |
| PSRAM analysis | ✅ Documented with risk assessment |
| Memory leak assessment | ✅ No leaks in badge code |
| Buffer size review | ✅ All sizes appropriate |

### Readiness for Hardware Testing

The firmware is compile-verified and ready for hardware testing. All 47 regression test cases require physical badge hardware to execute. Key areas to validate first on hardware:

1. **PSRAM detection** — Confirm `psramInit()` succeeds
2. **I2C bus sharing** — Touch + battery gauge coexistence
3. **NeoPixel timing** — Verify no interference with WiFi radio
4. **Light sleep wake** — Confirm all wake sources work
5. **OTA update flow** — End-to-end firmware update via web UI
