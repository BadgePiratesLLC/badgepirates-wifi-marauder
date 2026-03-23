# GitHub Issues — BSidesKC Badge WiFi Marauder Port

> Generated from `PORTING_PLAN.md`. Use `gh issue create` or paste into GitHub web UI.
> Labels to create first: `phase-1` through `phase-7`, `setup`, `hardware`, `wifi`, `ble`, `storage`, `integration`, `testing`, `blocker`, `research`

---

## Summary

| Phase | Issues | Description |
|-------|--------|-------------|
| 1 | 5 | Project Setup & Build System |
| 2 | 5 | Display & Input |
| 3 | 7 | WiFi Features |
| 4 | 4 | BLE Features |
| 5 | 5 | Storage & Persistence |
| 6 | 6 | Badge Integration |
| 7 | 4 | Testing & Polish |
| **Total** | **36** | |

---

## Phase 1: Project Setup & Build System

### Issue #1: [Phase 1] Fork/vendor ESP32Marauder source
**Labels:** `phase-1`, `setup`

**Description:**
Fork or vendor the ESP32Marauder source into this repository as the starting point for the BSidesKC badge port.

**Acceptance Criteria:**
- [ ] ESP32Marauder source is vendored into the repo (pinned to a known-good commit)
- [ ] Original license and attribution preserved
- [ ] Upstream commit SHA documented in README or VENDORED.md
- [ ] `.gitignore` updated for PlatformIO artifacts

**Notes:**
- Upstream: https://github.com/justcallmekoko/ESP32Marauder
- Closest board target: MARAUDER_V8 (ESP32-S3 + ILI9341 + touch + PSRAM)

---

### Issue #2: [Phase 1] Create PlatformIO configuration for BSidesKC badge
**Labels:** `phase-1`, `setup`

**Dependencies:** #1

**Description:**
Create `platformio.ini` targeting the BSidesKC badge hardware (ESP32-S3).

**Acceptance Criteria:**
- [ ] `platformio.ini` created with `esp32-s3-devkitc-1` board
- [ ] Flash configured: 16MB QIO @ 80MHz
- [ ] Partition table supports OTA and SPIFFS
- [ ] Build flags set for ESP32-S3 target
- [ ] All required library dependencies declared
- [ ] `pio run` completes without environment errors (link errors OK at this stage)

**Pin Reference:**
```
board = esp32-s3-devkitc-1
board_build.flash_mode = qio
board_build.f_flash = 80000000L
board_upload.flash_size = 16MB
```

---

### Issue #3: [Phase 1] Create BSIDESKC_BADGE board define in configs.h
**Labels:** `phase-1`, `setup`

**Dependencies:** #1

**Description:**
Add a new `BSIDESKC_BADGE` board definition to the Marauder config system with all badge-specific pin mappings.

**Acceptance Criteria:**
- [ ] `#define BSIDESKC_BADGE` added to configs.h (or equivalent)
- [ ] All pin mappings defined per porting plan:
  - Display SPI: MOSI(11), SCLK(12), CS(10), DC(5), RST(4), BL(6)
  - Touch I2C: SDA(8), SCL(9), RST(3), INT(7)
  - SD Card SPI: MOSI(35), SCK(36), MISO(37), CS(47)
  - NeoPixels: main(18), status(21)
  - Buzzer: 19
  - Buttons: Boot(0), Enter(38), Back(39)
  - Rotary Encoder: A(45), B(48), Button(20)
- [ ] Feature flags set (HAS_SD, HAS_NEOPIXEL, HAS_BATTERY, HAS_TOUCH, etc.)
- [ ] Conditional compilation guards work correctly

---

### Issue #4: [Phase 1] Configure TFT_eSPI User_Setup.h for badge display
**Labels:** `phase-1`, `setup`, `hardware`

**Dependencies:** #2

**Description:**
Create or modify `User_Setup.h` for TFT_eSPI to match the badge's ILI9341 display wiring.

**Acceptance Criteria:**
- [ ] ILI9341 driver selected (`#define ILI9341_DRIVER`)
- [ ] SPI pins configured: MOSI(11), SCLK(12), CS(10), DC(5), RST(4)
- [ ] Display dimensions: 320×240
- [ ] SPI frequency set appropriately for ESP32-S3
- [ ] Backlight pin (6) controlled via code
- [ ] Touch disabled in TFT_eSPI (handled separately via FT6336U)

---

### Issue #5: [Phase 1] Verify PSRAM presence and achieve clean compile
**Labels:** `phase-1`, `setup`, `research`, `blocker`

**Dependencies:** #2, #3, #4

**Description:**
Verify whether PSRAM is physically present on the BSidesKC badge PCB and achieve a clean compile of the project. PSRAM status is a blocker — it affects buffer sizes for packet capture, display buffers, and BLE stacks.

**Acceptance Criteria:**
- [ ] PSRAM presence verified on physical badge hardware
- [ ] If present: enable in platformio.ini (`board_build.psram = enabled`), document size
- [ ] If absent: document impact, set conservative buffer sizes
- [ ] Board JSON updated to reflect actual PSRAM status
- [ ] Project compiles cleanly with `pio run` (warnings OK, zero errors)
- [ ] Binary size documented and fits within flash partition

**Notes:**
Board JSON says no PSRAM, but it may be present. Physical inspection or runtime check (`ESP.getPsramSize()`) required.

---

## Phase 2: Display & Input

### Issue #6: [Phase 2] Verify ILI9341 display with Marauder rendering
**Labels:** `phase-2`, `hardware`

**Dependencies:** #4, #5

**Description:**
Flash the compiled firmware and verify the ILI9341 display renders Marauder's UI correctly.

**Acceptance Criteria:**
- [ ] Display initializes without errors
- [ ] Backlight turns on (GPIO 6)
- [ ] Marauder splash screen / boot screen renders
- [ ] Colors are correct (no R/B swap — RGB565 format)
- [ ] Orientation is correct (320×240 landscape)
- [ ] Text is readable and properly positioned
- [ ] No SPI bus conflicts with SD card

---

### Issue #7: [Phase 2] Implement FT6336U touch driver adapter
**Labels:** `phase-2`, `hardware`

**Dependencies:** #6

**Description:**
Replace the XPT2046 resistive touch driver with an FT6336U I2C capacitive touch adapter. Create a thin abstraction layer that translates FT6336U reads into the x/y/pressed format Marauder expects.

**Acceptance Criteria:**
- [ ] FT6336U library integrated (or minimal I2C driver written)
- [ ] I2C initialized on SDA(8), SCL(9) with RST(3) and INT(7)
- [ ] Touch adapter class implements Marauder's expected touch interface
- [ ] `getTouch(&x, &y)` returns correct coordinates mapped to 320×240
- [ ] Touch coordinates match display coordinates (no axis inversion issues)
- [ ] Interrupt pin (7) used for efficient polling (not busy-wait)
- [ ] I2C bus shared cleanly with MAX17048 battery gauge

**Notes:**
This is the touch adapter pattern described in the porting plan. The adapter must be a drop-in replacement so Marauder's UI code doesn't need modification.

---

### Issue #8: [Phase 2] Map hardware buttons to Marauder button interface
**Labels:** `phase-2`, `hardware`

**Dependencies:** #5

**Description:**
Map the badge's physical buttons to Marauder's navigation interface.

**Acceptance Criteria:**
- [ ] Enter button (GPIO 38) mapped to select/confirm action
- [ ] Back button (GPIO 39) mapped to back/cancel action
- [ ] Boot button (GPIO 0) mapped to secondary function (if needed)
- [ ] Debouncing implemented (hardware buttons need ~50ms debounce)
- [ ] Button presses register correctly in Marauder menus
- [ ] Long-press detection if required by Marauder UI

---

### Issue #9: [Phase 2] Map rotary encoder to menu scrolling
**Labels:** `phase-2`, `hardware`

**Dependencies:** #8

**Description:**
Map the rotary encoder to Marauder's menu scrolling system.

**Acceptance Criteria:**
- [ ] Encoder A(45) and B(48) configured with interrupt-driven reading
- [ ] Clockwise rotation = scroll down / next item
- [ ] Counter-clockwise rotation = scroll up / previous item
- [ ] Encoder button (GPIO 20) mapped to select/confirm (redundant with Enter)
- [ ] Rotation speed/sensitivity tuned for menu usability
- [ ] No missed steps or phantom rotations

---

### Issue #10: [Phase 2] End-to-end input validation
**Labels:** `phase-2`, `hardware`, `testing`

**Dependencies:** #7, #8, #9

**Description:**
Validate that all input methods (touch, buttons, encoder) work together to navigate Marauder's full menu system.

**Acceptance Criteria:**
- [ ] Can navigate to any menu item using touch alone
- [ ] Can navigate to any menu item using encoder + buttons alone
- [ ] Mixed input (touch + buttons) doesn't cause conflicts
- [ ] Menu scrolling is smooth and responsive
- [ ] No input lag > 100ms perceived

---

## Phase 3: WiFi Features

### Issue #11: [Phase 3] Verify promiscuous mode on ESP32-S3
**Labels:** `phase-3`, `wifi`, `blocker`

**Dependencies:** #5

**Description:**
Verify that WiFi promiscuous mode (monitor mode) works correctly on the ESP32-S3 platform. This is a prerequisite for all WiFi sniffing features.

**Acceptance Criteria:**
- [ ] `esp_wifi_set_promiscuous(true)` succeeds
- [ ] Promiscuous callback receives packets
- [ ] Management, data, and control frames are captured
- [ ] Channel hopping works correctly
- [ ] No crashes or watchdog resets during promiscuous operation

---

### Issue #12: [Phase 3] Test WiFi scanning
**Labels:** `phase-3`, `wifi`

**Dependencies:** #11

**Description:**
Verify Marauder's WiFi AP and station scanning works on the badge.

**Acceptance Criteria:**
- [ ] AP scan discovers nearby access points
- [ ] Station scan discovers connected clients
- [ ] SSID, BSSID, channel, RSSI displayed correctly
- [ ] Scan results populate Marauder's internal data structures
- [ ] Scan can be started and stopped from the menu
- [ ] Results display correctly on the 320×240 screen

---

### Issue #13: [Phase 3] Test packet monitoring
**Labels:** `phase-3`, `wifi`

**Dependencies:** #11

**Description:**
Verify WiFi packet monitoring/sniffing functionality.

**Acceptance Criteria:**
- [ ] Packet monitor mode starts and captures traffic
- [ ] Packet count and types displayed in real-time
- [ ] Channel can be locked or set to hop
- [ ] Monitor can be stopped cleanly without crash
- [ ] Memory usage stays stable during extended monitoring

---

### Issue #14: [Phase 3] Test deauthentication attacks
**Labels:** `phase-3`, `wifi`

**Dependencies:** #12

**Description:**
Verify deauthentication frame transmission works on ESP32-S3.

**Acceptance Criteria:**
- [ ] Deauth frames transmit successfully (verify with external monitor)
- [ ] Target selection from scan results works
- [ ] Single target and broadcast deauth both function
- [ ] Attack can be started and stopped from menu
- [ ] Status feedback displayed on screen

**Notes:**
ESP32-S3 may have different raw frame TX behavior than original ESP32. Test thoroughly.

---

### Issue #15: [Phase 3] Test beacon spam
**Labels:** `phase-3`, `wifi`

**Dependencies:** #11

**Description:**
Verify beacon frame generation and spam functionality.

**Acceptance Criteria:**
- [ ] Beacon spam mode starts and generates fake APs
- [ ] Custom SSID lists work
- [ ] Random SSID generation works
- [ ] Beacon frames visible on external WiFi scanner
- [ ] Can be stopped cleanly

---

### Issue #16: [Phase 3] Test PMKID/EAPOL capture
**Labels:** `phase-3`, `wifi`

**Dependencies:** #11, #19 (SD card for PCAP save)

**Description:**
Verify PMKID and EAPOL handshake capture functionality.

**Acceptance Criteria:**
- [ ] PMKID capture mode activates and listens
- [ ] EAPOL handshake frames are detected and captured
- [ ] Captured data can be saved to PCAP format
- [ ] Hash output compatible with hashcat/aircrack-ng
- [ ] Status display shows capture progress

**Notes:**
Full validation requires SD card (Phase 5) for PCAP saves. Basic capture can be tested without storage.

---

### Issue #17: [Phase 3] Test Evil Portal (AP mode)
**Labels:** `phase-3`, `wifi`

**Dependencies:** #11, #21 (SD for HTML storage)

**Description:**
Verify Evil Portal captive portal functionality.

**Acceptance Criteria:**
- [ ] Badge creates AP with configurable SSID
- [ ] Captive portal serves HTML page to connected clients
- [ ] DNS hijacking redirects all requests to portal
- [ ] Form submissions are captured and displayed
- [ ] Custom HTML templates load (from SD when available)
- [ ] Portal can be stopped and WiFi returned to normal mode

---

## Phase 4: BLE Features

### Issue #18: [Phase 4] Configure NimBLE 2 for ESP32-S3
**Labels:** `phase-4`, `ble`, `setup`

**Dependencies:** #5

**Description:**
Configure and initialize the NimBLE 2 BLE stack for the ESP32-S3 platform.

**Acceptance Criteria:**
- [ ] NimBLE 2 library added to dependencies
- [ ] BLE stack initializes without errors
- [ ] BLE and WiFi can coexist (no resource conflicts)
- [ ] Memory allocation for BLE is within budget (especially if no PSRAM)
- [ ] BLE can be enabled/disabled from Marauder menu

---

### Issue #19: [Phase 4] Test BLE scanning
**Labels:** `phase-4`, `ble`

**Dependencies:** #18

**Description:**
Verify BLE device scanning functionality.

**Acceptance Criteria:**
- [ ] BLE scan discovers nearby devices
- [ ] Device name, address, RSSI displayed
- [ ] Service UUIDs detected and shown
- [ ] Scan results display correctly on 320×240 screen
- [ ] Scan can be started/stopped from menu

---

### Issue #20: [Phase 4] Test BLE skimmer detection
**Labels:** `phase-4`, `ble`

**Dependencies:** #19

**Description:**
Verify BLE credit card skimmer detection feature.

**Acceptance Criteria:**
- [ ] Known skimmer signatures are checked during scan
- [ ] Suspicious devices flagged with visual alert
- [ ] Detection logic matches upstream Marauder behavior
- [ ] False positive rate is acceptable

---

### Issue #21: [Phase 4] Test BLE spam attacks
**Labels:** `phase-4`, `ble`

**Dependencies:** #18

**Description:**
Verify BLE advertisement spam functionality (e.g., FastPair spam, SwiftPair spam).

**Acceptance Criteria:**
- [ ] BLE spam modes available in menu
- [ ] Advertisement packets transmit at expected rate
- [ ] Spam visible on target devices (phone notifications, etc.)
- [ ] Can be stopped cleanly without BLE stack crash
- [ ] Multiple spam types work (if supported by upstream)

---

## Phase 5: Storage & Persistence

### Issue #22: [Phase 5] Configure SD card SPI interface
**Labels:** `phase-5`, `storage`, `hardware`

**Dependencies:** #5

**Description:**
Initialize and configure the SD card over SPI on the badge's dedicated SD SPI pins.

**Acceptance Criteria:**
- [ ] SD card SPI initialized: MOSI(35), SCK(36), MISO(37), CS(47)
- [ ] SD card detected and mounted on boot
- [ ] FAT32 filesystem readable and writable
- [ ] No SPI bus conflict with display (separate SPI bus or proper CS management)
- [ ] Graceful handling when no SD card is inserted
- [ ] Card capacity and free space readable

---

### Issue #23: [Phase 5] Test PCAP file saves to SD
**Labels:** `phase-5`, `storage`, `wifi`

**Dependencies:** #22, #13

**Description:**
Verify that captured WiFi packets can be saved as PCAP files to the SD card.

**Acceptance Criteria:**
- [ ] PCAP files written with correct header format
- [ ] Captured packets saved with timestamps
- [ ] Files openable in Wireshark and validate correctly
- [ ] File naming convention works (timestamps or sequential)
- [ ] Large captures don't corrupt filesystem
- [ ] Write performance sufficient for real-time capture

---

### Issue #24: [Phase 5] Test Evil Portal HTML storage on SD
**Labels:** `phase-5`, `storage`, `wifi`

**Dependencies:** #22, #17

**Description:**
Verify that Evil Portal HTML templates can be stored on and loaded from the SD card.

**Acceptance Criteria:**
- [ ] HTML files readable from SD card `/portal/` directory
- [ ] Multiple templates selectable from menu
- [ ] Template loads and serves correctly via Evil Portal
- [ ] Captured credentials saved to SD card

---

### Issue #25: [Phase 5] Test settings persistence
**Labels:** `phase-5`, `storage`

**Dependencies:** #22

**Description:**
Verify that Marauder settings persist across reboots.

**Acceptance Criteria:**
- [ ] Settings save to SD card (or SPIFFS)
- [ ] Settings load on boot
- [ ] Changed settings survive power cycle
- [ ] Corrupt settings file handled gracefully (reset to defaults)

---

### Issue #26: [Phase 5] Configure SPIFFS as fallback storage
**Labels:** `phase-5`, `storage`

**Dependencies:** #22

**Description:**
Configure SPIFFS as a fallback when SD card is not available.

**Acceptance Criteria:**
- [ ] SPIFFS partition defined in partition table
- [ ] SPIFFS initializes when SD card is absent
- [ ] Settings can be stored in SPIFFS
- [ ] Basic file operations work on SPIFFS
- [ ] Automatic fallback: try SD first, then SPIFFS
- [ ] User informed which storage backend is active

---

## Phase 6: Badge Integration

### Issue #27: [Phase 6] Add NeoPixel visual feedback
**Labels:** `phase-6`, `integration`, `hardware`

**Dependencies:** #5

**Description:**
Use the badge's 6× WS2812B NeoPixels (GPIO 18) and status LED (GPIO 21) to provide visual feedback for Marauder operations.

**Acceptance Criteria:**
- [ ] NeoPixel library initialized for 6 LEDs on GPIO 18
- [ ] Status LED on GPIO 21 initialized
- [ ] Visual patterns defined for:
  - Idle/ready state
  - WiFi scan active
  - Attack running (deauth, beacon, etc.)
  - BLE scan active
  - Capture in progress
  - Error/alert
- [ ] LED brightness configurable (battery conservation)
- [ ] LEDs can be disabled from settings menu

---

### Issue #28: [Phase 6] Add buzzer alerts
**Labels:** `phase-6`, `integration`, `hardware`

**Dependencies:** #5

**Description:**
Use the badge buzzer (GPIO 19) for audio feedback on key events.

**Acceptance Criteria:**
- [ ] Buzzer initialized on GPIO 19 with PWM/tone control
- [ ] Audio alerts for:
  - Menu selection confirmation
  - Scan complete
  - Handshake/PMKID captured
  - Skimmer detected
  - Error conditions
- [ ] Buzzer volume/enable configurable in settings
- [ ] Buzzer can be muted globally

---

### Issue #29: [Phase 6] Add battery level display from MAX17048
**Labels:** `phase-6`, `integration`, `hardware`

**Dependencies:** #7 (shares I2C bus with touch)

**Description:**
Read battery level from the MAX17048 fuel gauge and display it in the Marauder UI.

**Acceptance Criteria:**
- [ ] MAX17048 communicates over I2C (shared bus with FT6336U touch)
- [ ] Battery percentage read correctly
- [ ] Battery voltage read correctly
- [ ] Battery icon/percentage displayed in Marauder status bar
- [ ] Low battery warning at configurable threshold
- [ ] I2C bus sharing with touch controller is stable (no conflicts)

---

### Issue #30: [Phase 6] Menu integration with badge firmware
**Labels:** `phase-6`, `integration`

**Dependencies:** #10, #22

**Description:**
Investigate and optionally implement integration with the QACode_27 badge firmware for dual-boot or menu-launched Marauder.

**Acceptance Criteria:**
- [ ] Document feasibility of dual-boot approach
- [ ] If feasible: implement boot selector (hold button to choose firmware)
- [ ] If not feasible: document why and confirm standalone-only approach
- [ ] Boot time from power-on to Marauder menu documented

**Notes:**
Per porting plan: start standalone, dual-boot is stretch goal. This issue is for investigation and optional implementation.

---

### Issue #31: [Phase 6] OTA update support
**Labels:** `phase-6`, `integration`

**Dependencies:** #2 (partition table), #17 (AP mode)

**Description:**
Implement over-the-air firmware update capability.

**Acceptance Criteria:**
- [ ] OTA partition scheme configured in partition table
- [ ] OTA update can be triggered from Marauder menu
- [ ] Firmware binary served via WiFi AP or downloaded from URL
- [ ] Update progress displayed on screen
- [ ] Failed update rolls back to previous firmware
- [ ] OTA update verified with checksum

---

### Issue #32: [Phase 6] Power management optimization
**Labels:** `phase-6`, `integration`

**Dependencies:** #29

**Description:**
Optimize power consumption for battery-powered badge operation.

**Acceptance Criteria:**
- [ ] Display backlight dimming after inactivity timeout
- [ ] NeoPixel brightness auto-reduces on low battery
- [ ] WiFi/BLE radios disabled when not in use
- [ ] Sleep mode available from menu
- [ ] Estimated battery life documented for common usage patterns

---

## Phase 7: Testing & Polish

### Issue #33: [Phase 7] Full feature regression test
**Labels:** `phase-7`, `testing`

**Dependencies:** All Phase 3, 4, 5, 6 issues

**Description:**
Comprehensive regression test of all Marauder features on the BSidesKC badge.

**Acceptance Criteria:**
- [ ] **WiFi Features:**
  - [ ] AP scanning works
  - [ ] Station scanning works
  - [ ] Packet monitoring works
  - [ ] Deauthentication works
  - [ ] Beacon spam works
  - [ ] PMKID capture works
  - [ ] EAPOL capture works
  - [ ] Evil Portal works
- [ ] **BLE Features:**
  - [ ] BLE scanning works
  - [ ] Skimmer detection works
  - [ ] BLE spam works
- [ ] **Storage:**
  - [ ] SD card read/write works
  - [ ] PCAP saves work
  - [ ] Settings persist
  - [ ] SPIFFS fallback works
- [ ] **Input:**
  - [ ] Touch works
  - [ ] Buttons work
  - [ ] Rotary encoder works
- [ ] **Integration:**
  - [ ] NeoPixels work
  - [ ] Buzzer works
  - [ ] Battery display works
  - [ ] OTA works
- [ ] Test results documented in TESTING.md

---

### Issue #34: [Phase 7] Memory profiling and optimization
**Labels:** `phase-7`, `testing`

**Dependencies:** #5, #33

**Description:**
Profile memory usage and optimize for the badge's available RAM (with or without PSRAM).

**Acceptance Criteria:**
- [ ] Free heap documented at boot and during each major feature
- [ ] PSRAM usage documented (if available)
- [ ] Stack high-water marks checked for all tasks
- [ ] No memory leaks during extended operation (1+ hour test)
- [ ] Buffer sizes optimized for available memory
- [ ] Crash-free operation under memory pressure scenarios
- [ ] Results documented in MEMORY_PROFILE.md

---

### Issue #35: [Phase 7] Power consumption testing
**Labels:** `phase-7`, `testing`

**Dependencies:** #32

**Description:**
Measure and document power consumption across different operating modes.

**Acceptance Criteria:**
- [ ] Current draw measured for:
  - [ ] Idle (menu displayed, no radio activity)
  - [ ] WiFi scanning
  - [ ] WiFi attack (deauth/beacon)
  - [ ] BLE scanning
  - [ ] Full NeoPixel brightness
  - [ ] Display off / sleep mode
- [ ] Battery life estimates calculated for each mode
- [ ] Results documented in POWER_PROFILE.md

---

### Issue #36: [Phase 7] UI polish for 320×240 display
**Labels:** `phase-7`, `testing`

**Dependencies:** #6, #33

**Description:**
Polish the Marauder UI for optimal display on the badge's 320×240 landscape screen.

**Acceptance Criteria:**
- [ ] All menu items visible without horizontal scrolling
- [ ] Font sizes appropriate for 320×240 at typical viewing distance
- [ ] Status bar fits all info (battery, WiFi status, mode indicator)
- [ ] Long text truncated with ellipsis, not clipped
- [ ] Color scheme readable in various lighting conditions
- [ ] Touch targets large enough for finger input (minimum 40×40px)
- [ ] Screenshots of final UI captured for documentation

---

## Quick Reference: Label Setup

Run these commands to create all labels before importing issues:

```bash
gh label create "phase-1" --color "1d76db" --description "Phase 1: Project Setup & Build System"
gh label create "phase-2" --color "0e8a16" --description "Phase 2: Display & Input"
gh label create "phase-3" --color "d93f0b" --description "Phase 3: WiFi Features"
gh label create "phase-4" --color "5319e7" --description "Phase 4: BLE Features"
gh label create "phase-5" --color "fbca04" --description "Phase 5: Storage & Persistence"
gh label create "phase-6" --color "f9d0c4" --description "Phase 6: Badge Integration"
gh label create "phase-7" --color "c5def5" --description "Phase 7: Testing & Polish"
gh label create "setup" --color "bfdadc" --description "Build system and project configuration"
gh label create "hardware" --color "d4c5f9" --description "Hardware drivers and pin configuration"
gh label create "wifi" --color "ff6666" --description "WiFi features and attacks"
gh label create "ble" --color "7057ff" --description "Bluetooth Low Energy features"
gh label create "storage" --color "ffd700" --description "SD card, SPIFFS, file I/O"
gh label create "integration" --color "ff9ecf" --description "Badge-specific integration features"
gh label create "testing" --color "0075ca" --description "Testing and validation"
gh label create "blocker" --color "b60205" --description "Blocks other work"
gh label create "research" --color "d876e3" --description "Requires investigation or hardware verification"
```

## Quick Reference: Issue Creation via CLI

Example for creating a single issue:
```bash
gh issue create \
  --title "[Phase 1] Fork/vendor ESP32Marauder source" \
  --body "$(cat <<'EOF'
## Description
Fork or vendor the ESP32Marauder source into this repository.

## Acceptance Criteria
- [ ] ESP32Marauder source vendored (pinned commit)
- [ ] License and attribution preserved
- [ ] Upstream commit SHA documented
- [ ] .gitignore updated for PlatformIO
EOF
)" \
  --label "phase-1,setup"
```
