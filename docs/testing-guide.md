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

## Phase 2 Validation Checklist

| # | Test | Pass? |
|---|------|-------|
| 1 | `pio run -e bsideskc-badge` compiles clean | |
| 2 | Display shows boot splash on power-up | |
| 3 | Backlight turns on after splash | |
| 4 | BOOT short-press cycles backlight | |
| 5 | Backlight level persists after reboot | |
| 6 | Touch test mode: dots drawn at touch points | |
| 7 | Touch coordinates correct (not inverted/mirrored) | |
| 8 | ENTER button navigates menus | |
| 9 | BACK button goes back in menus | |
| 10 | Encoder CW scrolls menu down | |
| 11 | Encoder CCW scrolls menu up | |
| 12 | Encoder button selects menu item | |
| 13 | Full validation test: 7/7 inputs pass | |
| 14 | Menu wraps correctly at top/bottom | |
| 15 | Serial output shows all input events | |

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
| WiFi scan | 3 | |
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
