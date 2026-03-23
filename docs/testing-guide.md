# Testing Guide — BSidesKC Marauder Port

## Build Verification
1. Compile: `pio run -e bsideskc-badge` — must succeed with zero errors
2. Static analysis: review all `#ifdef BSIDESKC_BADGE` paths for correctness
3. Pin conflict audit: verify no GPIO used by two peripherals

## Hardware Smoke Test
1. Flash firmware via USB
2. Display shows Marauder boot screen
3. Touch input registers (check serial monitor for coordinates)
4. SD card mounts and lists files
5. WiFi scan returns nearby APs

---

## Phase 2: Display & Input Test Procedures

### Test Mode Entry

The firmware provides three test modes accessible by holding buttons during boot:

| Hold during boot | Test mode | Exit method |
|------------------|-----------|-------------|
| **BOOT** (GPIO 0) | Full input validation | Hold BOOT + BACK together |
| **BACK** (GPIO 39) | Encoder-only test | Hold ENTER |
| **ENTER** (GPIO 38) | Touch-only test | Hold BACK |

Priority order: BOOT is checked first, then BACK, then ENTER.

### Full Input Validation Test (Issue #34)

**Entry:** Hold BOOT button while powering on or resetting the badge.

**Screen layout:**
- Left column: checklist of 7 input components (unchecked → checked as validated)
- Right column: live encoder position, backlight level, touch coordinates
- Bottom: exit instructions

**Procedure:**
1. Power on while holding BOOT
2. Touch the screen anywhere → `[X] Touch` checks off, coordinates shown
3. Press ENTER button → `[X] ENTER btn` checks off
4. Press BACK button → `[X] BACK btn` checks off
5. Press BOOT button (short press) → `[X] BOOT btn` checks off
6. Rotate encoder clockwise → `[X] Encoder CW` checks off, position increments
7. Rotate encoder counter-clockwise → `[X] Encoder CCW` checks off, position decrements
8. Press encoder knob → `[X] Encoder Btn` checks off
9. Long-press BOOT → backlight cycles through 10 levels (display validation)

**Expected serial output:** Each input event prints `[InputTest] <event>` to serial at 115200 baud.

**Exit:** Hold BOOT + BACK simultaneously. Summary screen shows pass count (7/7 = ALL PASS).

### Encoder Test Mode

**Entry:** Hold BACK button during boot (if BOOT is not held).

**Procedure:**
1. Rotate encoder — position counter updates on screen
2. Press encoder button — logged to serial
3. Hold ENTER to exit

### Touch Test Mode

**Entry:** Hold ENTER button during boot (if neither BOOT nor BACK is held).

**Procedure:**
1. Tap screen — cyan dots drawn at touch points, coordinates shown
2. Hold BACK to exit

### Display & Backlight Validation

During normal operation:
- Short-press BOOT to cycle backlight through 10 brightness levels
- Backlight level persists across reboots (stored in NVS)
- Verify boot splash shows "BSidesKC Badge" / "ESP32 Marauder" / version

### Button Navigation (Normal Operation)

| Button | Menu mode action | Scan mode action |
|--------|-----------------|------------------|
| ENTER (GPIO 38) | Select menu item | — |
| BACK (GPIO 39) | Go back / exit | Stop scan |
| BOOT (GPIO 0) | Cycle backlight | Cycle backlight |
| Encoder CW | Scroll down | — |
| Encoder CCW | Scroll up | — |
| Encoder press | Select menu item | — |

---

## Phase 3: WiFi Scan Test Procedures

### WiFi Scan Test Mode (Issue #36)

**Entry:** Hold ENTER + BACK simultaneously during boot.

**Priority:** This combo is checked first, before single-button test modes.

**What it tests:**
1. `WiFi.scanNetworks()` — synchronous scan for all APs (including hidden)
2. Result display — SSID, RSSI, channel, encryption type
3. Channel hopping — `esp_wifi_set_channel()` on channels 1, 6, 11
4. Scan performance — elapsed time in milliseconds

**Screen layout:**
- Title: "WiFi Scan Test"
- Summary: "Found N APs in Xms"
- Channel hop status: OK / FAIL
- AP list (color-coded by signal strength):
  - Green: > -50 dBm (excellent)
  - Yellow: -50 to -70 dBm (good)
  - Orange: -70 to -85 dBm (fair)
  - Red: < -85 dBm (weak)
- Each row: `RSSIdBm chNN ENC   SSID`

**Expected serial output:**
```
[WiFiTest] Starting scan...
[WiFiTest] Found N APs in Xms
[WiFiTest] Chan hop: OK
[WiFiTest] -42dBm ch06 WPA2  MyNetwork
[WiFiTest] Done. N APs, Xms, hop=OK
```

**Exit:** Hold BACK button. WiFi is turned off on exit.

### Marauder Menu WiFi Scan (Issue #36)

The upstream Marauder menu system provides WiFi scanning via:
**WiFi → Scan APs**

**Procedure:**
1. Boot normally (no buttons held)
2. Navigate: WiFi → Scan APs (use encoder or touch)
3. Scan starts — APs appear on screen in real-time
4. Press BACK to stop scan
5. Verify AP list shows SSID, RSSI, channel info
6. Navigate: WiFi → Select APs — verify scanned APs are selectable

**What to verify:**
- Scan starts without crash
- APs populate on screen
- BACK button stops the scan
- Returning to menu works cleanly
- Re-scanning works (no stale state)

### Boot Test Mode Priority

| Priority | Hold during boot | Test mode | Exit method |
|----------|------------------|-----------|-------------|
| 1 | **ENTER + BACK** | WiFi scan test | Hold BACK |
| 2 | **BOOT** (GPIO 0) | Full input validation | Hold BOOT + BACK |
| 3 | **BACK** (GPIO 39) | Encoder-only test | Hold ENTER |
| 4 | **ENTER** (GPIO 38) | Touch-only test | Hold BACK |

---

## Phase 3 Validation Checklist

| # | Test | Pass? |
|---|------|-------|
| 1 | `pio run -e bsideskc-badge` compiles clean | |
| 2 | WiFi scan test mode enters (ENTER+BACK at boot) | |
| 3 | WiFi.scanNetworks() finds nearby APs | |
| 4 | Scan results display SSID, RSSI, channel, encryption | |
| 5 | Signal strength color coding works | |
| 6 | Channel hopping reports OK | |
| 7 | Scan completes in reasonable time (<5s) | |
| 8 | BACK button exits test mode cleanly | |
| 9 | WiFi turned off after test mode exit | |
| 10 | Marauder menu: WiFi → Scan APs starts scan | |
| 11 | Marauder menu: BACK stops scan | |
| 12 | Marauder menu: Select APs shows scanned results | |
| 13 | Serial output shows all scan events | |
| 14 | No crash on repeated scans | |
| 15 | Hidden networks shown as "(hidden)" | |

---

## Feature Regression Matrix

| Feature | Phase | Status |
|---------|-------|--------|
| Display init | 2 | ✅ |
| Backlight control | 2 | ✅ |
| Touch input | 2 | ✅ |
| Button navigation | 2 | ✅ |
| Rotary encoder | 2 | ✅ |
| Input validation test | 2 | ✅ |
| WiFi scan | 3 | ✅ |
| Packet monitor | 3 | |
| Deauth | 3 | |
| Beacon spam | 3 | |
| PMKID capture | 3 | |
| Evil Portal | 3 | |
| BLE scan | 4 | |
| BLE skimmer detect | 4 | |
| SD PCAP save | 5 | |
| Settings persist | 5 | |
| NeoPixel feedback | 6 | |
| Battery display | 6 | |

## Known Issues
_(Track issues here as they arise)_
