# Hardware Testing Checklist

## BSidesKC Badge WiFi Marauder — Hardware Validation

**Last Updated:** March 24, 2026
**Status:** ✅ Core validation complete — boot, display, input, WiFi init verified

---

## Priority 1: Boot & Display

- ✅ **Flash firmware** — `pio run --target upload` succeeds (after flash size fix: 8MB DIO)
- ✅ **Serial output** — `pio device monitor -b 115200` shows full boot log with debug markers
- ✅ **Display init** — ILI9341 shows splash screen → Marauder UI
- ✅ **Backlight** — Display backlight turns on via PWM on GPIO 6
- ✅ **Orientation** — Landscape 320×240 (fixed: `SCREEN_ORIENTATION` 0→1)
- ✅ **Colors** — UI colors render correctly, no R/B swap

**Notes:**
- Required flash settings change: 16MB QIO → 8MB DIO with `default_8MB.csv` partition table
- PSRAM disabled — badge has no PSRAM; `ps_malloc()` caused StoreProhibited crash
- USB CDC disabled (`ARDUINO_USB_CDC_ON_BOOT=0`) to prevent serial blocking without USB host
- Status LED (GPIO 21) blinks 5× immediately on boot to confirm code execution

---

## Priority 2: Touch & Input

- ✅ **Touch detected** — FT6336U reports touch events via XPT2046 shim
- ✅ **Touch coordinates** — X/Y mapping corrected (ADC ranges: X 216–3786, Y 143–3715)
- ✅ **Touch calibration** — UI buttons respond to correct screen regions after mapping fix
- ✅ **Rotary encoder** — Rotation generates up/down menu navigation
- ✅ **Encoder button** — Press registers as select/enter
- ✅ **Button debounce** — No double-triggers observed

**Notes:**
- Touch coordinate fix was critical — original mapping ranges (200–3700, 240–3800) were off
- Debug logging (`[Touch] raw FT6336U x=... y=...`) still active — remove for release build
- Encoder test mode (hold BACK during boot) confirmed working

---

## Priority 3: WiFi Scanning

- ✅ **WiFi scan starts** — Scan mode initiates without crash (after WiFi PSRAM patch)
- ✅ **APs detected** — Nearby access points appear in scan results
- ✅ **RSSI values** — Signal strength readings are reasonable
- ✅ **Channel display** — Correct channel numbers shown
- [ ] **Channel hopping** — Needs extended testing
- [ ] **Scan results persist** — Needs extended testing

**Notes:**
- Required `wifi_patch.h` to disable PSRAM TX cache buffers (`cache_tx_buf_num = 0`)
- WiFi init was the last crash point after PSRAM fixes
- `mac_history_len` reduced 500→50 for memory constraints

---

## Priority 4: WiFi Attacks

- [ ] **Deauth** — Pending field testing
- [ ] **Beacon spam** — Pending field testing
- [ ] **PMKID capture** — Pending field testing
- [ ] **EAPOL capture** — Pending field testing
- [ ] **Evil Portal** — Pending (MAX_HTML_SIZE reduced to 8KB)
- [ ] **PCAP save** — Pending (depends on SD card)

**Notes:**
- Evil Portal HTML buffer reduced from 30KB to 8KB to fit in SRAM
- All attack features compile-verified; runtime testing pending

---

## Priority 5: BLE Features

- [ ] **BLE scan starts** — Pending testing
- [ ] **Devices detected** — Pending testing
- [ ] **Skimmer detection** — Pending testing
- [ ] **BLE spam** — Pending testing
- [ ] **WiFi/BLE coexistence** — Pending testing

---

## Priority 6: Storage

- ❌ **SD card detected** — "SD Card NOT Supported" on boot (needs FAT32 card testing)
- [ ] **SD read/write** — Blocked by SD detection
- ✅ **SPIFFS mount** — SPIFFS partition mounts as fallback
- [ ] **SPIFFS read/write** — Needs explicit testing
- [ ] **PCAP files** — Blocked by storage validation
- ✅ **Settings persist** — `settings_obj.begin()` completes without error
- [ ] **Evil Portal HTML** — Pending

**Notes:**
- SD card SPI bus uses dedicated pins (MOSI:35, SCK:36, MISO:37, CS:47)
- Need to test with known-good FAT32 SD card
- SPIFFS available as fallback storage

---

## Priority 7: Badge Integration

- ✅ **NeoPixels** — LEDs light up with correct colors
- ✅ **LED state mapping** — Colors change with Marauder state (idle/scan/attack)
- ✅ **Buzzer** — Tones play on button press events
- [ ] **Buzzer volume** — Needs subjective assessment
- ✅ **Battery reading** — `battery_obj.RunSetup()` completes (MAX17048 fuel gauge)
- [ ] **Battery percentage** — Needs calibration with actual battery
- ✅ **Badge menu** — Menu renders, navigation works via encoder + touch
- ✅ **Marauder launch** — Launching Marauder from badge menu works
- [ ] **Return to menu** — Needs testing

**Notes:**
- LED update rate reduced from 30fps to 20fps to prevent display flicker
- NeoPixel `show()` disables interrupts; lower rate mitigates SPI contention

---

## Priority 8: Power & OTA

- [ ] **Current draw (active)** — Needs multimeter measurement
- [ ] **Current draw (idle)** — Needs measurement
- [ ] **Light sleep** — Power manager initialized; needs validation
- [ ] **Sleep current** — Needs measurement
- [ ] **Battery life** — Needs estimation from measurements
- [ ] **OTA server** — Pending testing
- [ ] **OTA flash** — Pending testing
- [ ] **OTA verify** — Pending testing

---

## Final Validation

- [ ] **Extended run** — 1+ hour continuous operation
- [ ] **Memory stability** — Heap leak monitoring over time
- [ ] **All features cycle** — Sequential feature walkthrough
- ✅ **Power cycle** — Clean boot after hard power off confirmed
- [ ] **Multiple badges** — Pending additional hardware

---

## Boot Fixes Applied (Required for Hardware)

| Fix | Commit | Impact |
|-----|--------|--------|
| Flash 16MB→8MB, QIO→DIO | `965785f` | Device wouldn't boot at all |
| Disable HAS_PSRAM | `420f1fc` | StoreProhibited crash loop |
| Reduce mac_history 500→50 | `740efe7` | Memory exhaustion |
| Reduce MAX_HTML_SIZE 30K→8K | `a5a7cd1` | BSS overflow |
| WiFi PSRAM cache patch | `f1c499a` | Crash during WiFi init |
| Display orientation 0→1 | `386d75a` | Portrait instead of landscape |
| Touch coordinate mapping | `386d75a` | Taps hit wrong UI elements |
| LED rate 30→20fps | `386d75a` | Display flicker |
| USB CDC off | `965785f` | Serial blocking without USB host |

---

## Sign-Off

| Test Area | Tester | Date | Status | Notes |
|-----------|--------|------|--------|-------|
| Boot & Display | Dev team | Mar 24, 2026 | ✅ Pass | After 6 boot fixes |
| Touch & Input | Dev team | Mar 24, 2026 | ✅ Pass | Coordinate mapping fixed |
| WiFi Scanning | Dev team | Mar 24, 2026 | ✅ Pass | WiFi PSRAM patch required |
| WiFi Attacks | — | — | ⏳ Pending | Compile-verified only |
| BLE Features | — | — | ⏳ Pending | Compile-verified only |
| Storage | Dev team | Mar 24, 2026 | ⚠️ Partial | SD card not detected |
| Badge Integration | Dev team | Mar 24, 2026 | ✅ Pass | LEDs, buzzer, menu working |
| Power & OTA | — | — | ⏳ Pending | Needs measurement |
| Final Validation | — | — | ⏳ Pending | Extended testing needed |
