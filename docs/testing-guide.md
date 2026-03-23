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

## Phase 3: Upstream Attack Feature Verification

### Compile Verification (Issues #38-41)

All upstream WiFi attack features compile for ESP32-S3. Hardware testing is required to confirm runtime behavior.

### Deauthentication Test (Issue #38)

**Menu path:** WiFi → Scan APs → Select APs → Deauth

**Procedure:**
1. Scan for APs and select a target
2. Start deauth attack from menu
3. Verify serial output shows `esp_wifi_80211_tx()` calls
4. Verify target clients disconnect (requires monitoring device)
5. Press BACK to stop attack

**What to verify on hardware:**
- Raw frame injection via `esp_wifi_80211_tx()` succeeds
- Deauth frames are actually transmitted (use second device to monitor)
- Attack stops cleanly on BACK press
- No crash on repeated start/stop cycles

### Beacon Spam Test (Issue #39)

**Menu path:** WiFi → Beacon Spam → (Random/List/Target)

**Procedure:**
1. Select beacon spam mode (Random recommended for first test)
2. Verify serial output shows beacon frame construction
3. Use a phone/laptop to see fake SSIDs appear in WiFi list
4. Press BACK to stop

**What to verify on hardware:**
- Fake SSIDs visible on nearby devices
- Frame rate is reasonable (not flooding serial)
- Clean stop and return to menu

### PMKID/EAPOL Capture Test (Issue #40)

**Menu path:** WiFi → Scan APs → Select APs → PMKID Scan

**Procedure:**
1. Scan and select a WPA2 target AP
2. Start PMKID/EAPOL scan
3. Force a client reconnection to the target AP
4. Verify `eapolSnifferCallback` fires (serial output)
5. If SD card is available, verify PCAP file written

**What to verify on hardware:**
- EAPOL frames captured in promiscuous callback
- PMKID extracted from first message of 4-way handshake
- PCAP file contains valid capture (Phase 5 dependency)

### Evil Portal Test (Issue #41)

**Menu path:** WiFi → Evil Portal → (select portal)

**Procedure:**
1. Start Evil Portal from menu
2. Verify AP starts (check with phone WiFi scan)
3. Connect a client device to the portal AP
4. Verify captive portal page loads in browser
5. Submit test data and verify capture on serial
6. Press BACK to stop portal

**What to verify on hardware:**
- AP mode activates and is visible to clients
- DNS redirect works (captive portal auto-opens)
- HTML portal page renders correctly
- Form submissions captured and logged
- Clean shutdown on exit

### Phase 3 Attack Feature Compile Checklist

| # | Test | Pass? |
|---|------|-------|
| 1 | Deauth code compiles (`WiFiScan::RunDeauth`) | ✅ |
| 2 | Beacon spam compiles (`WiFiScan::RunBeaconSpam`) | ✅ |
| 3 | Probe spam compiles (`WiFiScan::RunProbeSpam`) | ✅ |
| 4 | EAPOL scan compiles (`WiFiScan::RunEapolScan`) | ✅ |
| 5 | Evil Portal compiles (`EvilPortal::setup/begin`) | ✅ |
| 6 | Pineapple scan compiles (`WiFiScan::RunPineappleScan`) | ✅ |
| 7 | Raw TX compiles (`esp_wifi_80211_tx`) | ✅ |
| 8 | Full build succeeds with zero WiFi errors | ✅ |

---

## WiFi Feature Testing — Responsible Use

> **This firmware is a port of the open-source ESP32Marauder security research
> tool. WiFi features are intended for authorized security research and
> education only.**

### Legal and Ethical Requirements

- **Only test on networks you own or have explicit written authorization to test**
- Unauthorized use of deauth, beacon spam, or Evil Portal features against
  networks you do not own is illegal in most jurisdictions
- Comply with all applicable local, state, and federal laws (e.g., CFAA in the
  US, Computer Misuse Act in the UK)
- PMKID/EAPOL capture should only target your own access points
- Evil Portal testing must not be used to collect credentials from
  unsuspecting users
- Use an RF-shielded environment or Faraday cage when possible to avoid
  affecting neighboring networks

### Testing Environment Recommendations

- Use a dedicated test AP that you own (isolated from production networks)
- Disable the test AP's internet uplink during testing
- Use a non-overlapping channel to minimize interference
- Keep a log of all testing activities
- Power down WiFi attack features immediately after testing

---

## Phase 4: BLE Test Procedures

### BLE Scan Test (Issue #43)

**Menu path:** Bluetooth → BLE Scan (or `btscan` via CLI)

**Procedure:**
1. Boot normally, navigate to Bluetooth → BLE Scan
2. Verify scan starts — nearby BLE devices appear on screen
3. Check serial output for device addresses, RSSI, and names
4. Press BACK to stop scan
5. Verify clean return to menu

**What to verify on hardware:**
- NimBLE scan initializes without crash
- Devices discovered with correct addresses and RSSI
- Scan stops cleanly on BACK press
- Re-scanning works (no stale state)
- Memory stable after repeated scans

### BLE Skimmer Detection Test (Issue #44)

**Menu path:** Bluetooth → Skimmer Detect (or `btscan -t skimmer` via CLI)

**Procedure:**
1. Navigate to Bluetooth → Skimmer Detect
2. Scan runs and filters for known skimmer signatures
3. Verify serial output shows payload pattern matching
4. If a known skimmer device is available, verify detection alert
5. Press BACK to stop

**What to verify on hardware:**
- Skimmer signature matching against BLE advertisement payloads
- Alert display when skimmer-like device found
- No false positives on common consumer BLE devices
- Clean stop and return to menu

### BLE Spam Attack Tests (Issue #45)

**Menu path:** Bluetooth → BLE Spam → (select type)

| Spam Type | Target | CLI Command |
|-----------|--------|-------------|
| Sour Apple | Apple devices (popup spam) | `blespam -t apple` |
| SwiftPair | Windows devices | `blespam -t windows` |
| Samsung | Samsung devices | `blespam -t samsung` |
| Google Fast Pair | Android devices | `blespam -t google` |
| Flipper | Flipper Zero | `blespam -t flipper` |
| Spam All | All of the above | `blespam -t all` |

**Procedure:**
1. Select a spam type from the menu
2. Verify serial output shows advertisement construction and transmission
3. Use a target device (phone/laptop) to verify spam popups appear
4. Press BACK to stop
5. Verify advertising stops and menu returns cleanly

**What to verify on hardware:**
- BLE advertisements transmit successfully
- Target devices receive and display spam notifications
- Attack stops immediately on BACK press
- No crash on repeated start/stop cycles
- WiFi still functional after BLE spam (coexistence)

### AirTag Scan/Spoof Tests (Issues #43, #45)

**AirTag Scan — Menu path:** Bluetooth → AirTag Scan

**Procedure:**
1. Start AirTag scan with a real AirTag nearby
2. Verify detection and Apple continuity protocol parsing
3. Check serial output for AirTag-specific data

**AirTag Spoof — Menu path:** Bluetooth → Spoof AirTag (or `spoofat` via CLI)

**Procedure:**
1. Start AirTag spoof
2. Use an Apple device to check if spoofed AirTag appears in Find My
3. Press BACK to stop

### Phase 4 BLE Compile Checklist

| # | Test | Pass? |
|---|------|-------|
| 1 | `pio run -e bsideskc-badge` compiles with BLE enabled | ✅ |
| 2 | NimBLE 1.4.3 links successfully for ESP32-S3 | ✅ |
| 3 | BLE scan code compiles (`WiFiScan::RunBleScan`) | ✅ |
| 4 | Skimmer detect compiles (`BT_SCAN_SKIMMERS` path) | ✅ |
| 5 | AirTag scan compiles (`BT_SCAN_AIRTAG` path) | ✅ |
| 6 | Flipper scan compiles (`BT_SCAN_FLIPPER` path) | ✅ |
| 7 | Sour Apple compiles (`BT_ATTACK_SOUR_APPLE` path) | ✅ |
| 8 | SwiftPair spam compiles (`BT_ATTACK_SWIFTPAIR_SPAM`) | ✅ |
| 9 | Samsung spam compiles (`BT_ATTACK_SAMSUNG_SPAM`) | ✅ |
| 10 | Google spam compiles (`BT_ATTACK_GOOGLE_SPAM`) | ✅ |
| 11 | Flipper spam compiles (`BT_ATTACK_FLIPPER_SPAM`) | ✅ |
| 12 | AirTag spoof compiles (`BT_SPOOF_AIRTAG`) | ✅ |
| 13 | Full build succeeds with zero BLE errors | ✅ |

### Phase 4 Hardware Test Checklist (Pending)

| # | Test | Pass? |
|---|------|-------|
| 1 | BLE scan discovers nearby devices | |
| 2 | Device names and RSSI displayed correctly | |
| 3 | Skimmer detection filters correctly | |
| 4 | AirTag detected when nearby | |
| 5 | BLE spam popups appear on target device | |
| 6 | AirTag spoof visible in Find My | |
| 7 | BLE + WiFi coexistence stable | |
| 8 | Memory stable under BLE scan load | |
| 9 | Repeated scan start/stop without crash | |
| 10 | BACK button stops all BLE operations cleanly | |

---

## BLE Feature Testing — Responsible Use

> **BLE spam and spoofing features are intended for authorized security
> research and education only.**

### Legal and Ethical Requirements

- Only test BLE spam attacks in controlled environments you own
- Do not use BLE spam in public spaces — it affects all nearby devices
- AirTag spoofing may violate Apple's terms of service
- Comply with all applicable wireless communication laws
- Use an RF-shielded environment when possible

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
| Packet monitor | 3 | ✅ compile-verified |
| Deauth | 3 | ✅ compile-verified |
| Beacon spam | 3 | ✅ compile-verified |
| PMKID capture | 3 | ✅ compile-verified |
| Evil Portal | 3 | ✅ compile-verified |
| BLE scan | 4 | ✅ compile-verified |
| BLE skimmer detect | 4 | ✅ compile-verified |
| BLE spam attacks | 4 | ✅ compile-verified |
| AirTag scan/spoof | 4 | ✅ compile-verified |
| Flipper/Flock detect | 4 | ✅ compile-verified |
| SD PCAP save | 5 | |
| Settings persist | 5 | |
| NeoPixel feedback | 6 | |
| Battery display | 6 | |

## Known Issues
_(Track issues here as they arise)_
