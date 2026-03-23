# Hardware Testing Checklist

## BSidesKC Badge WiFi Marauder — Hardware Validation

Use this checklist when physical badges are available. Tests are ordered by priority — earlier items block later ones.

---

## Priority 1: Boot & Display

- [ ] **Flash firmware** — `pio run --target upload` succeeds
- [ ] **Serial output** — `pio device monitor -b 115200` shows boot log
- [ ] **Display init** — ILI9341 shows splash screen / Marauder UI
- [ ] **Backlight** — Display backlight turns on at expected brightness
- [ ] **Orientation** — Display renders in correct landscape orientation (320×240)
- [ ] **Colors** — UI colors render correctly (no R/B swap)

**Troubleshooting:**
- Blank screen → Check SPI pins in `bsideskc_pins.h`, verify TFT_CS/DC/RST
- Wrong colors → Toggle `TFT_RGB_ORDER` in board config
- Garbled display → Verify SPI clock speed (40MHz default, try 20MHz)

---

## Priority 2: Touch & Input

- [ ] **Touch detected** — FT6336U reports touch events on serial
- [ ] **Touch coordinates** — X/Y values map correctly to display pixels
- [ ] **Touch calibration** — UI buttons respond to correct screen regions
- [ ] **Rotary encoder** — Rotation generates up/down navigation
- [ ] **Encoder button** — Press registers as select/enter
- [ ] **Button debounce** — No double-triggers on single press

**Troubleshooting:**
- No touch → Check I2C address (0x38 default), verify SDA/SCL pins
- Inverted axes → Swap X/Y or invert axis in touch adapter
- Encoder skips → Adjust debounce timing in `encoder_handler.h`

---

## Priority 3: WiFi Scanning

- [ ] **WiFi scan starts** — Scan mode initiates without crash
- [ ] **APs detected** — Nearby access points appear in scan results
- [ ] **RSSI values** — Signal strength readings are reasonable
- [ ] **Channel display** — Correct channel numbers shown
- [ ] **Channel hopping** — Scanner cycles through channels 1–13
- [ ] **Scan results persist** — Results remain after scan completes

**Troubleshooting:**
- No APs found → Verify WiFi antenna connection on badge PCB
- Crash on scan → Check heap memory, reduce scan buffer size
- Weak signals → ESP32-S3 antenna may need ground plane check

---

## Priority 4: WiFi Attacks

- [ ] **Deauth** — Target AP/client deauthentication works
- [ ] **Beacon spam** — Fake APs appear on nearby devices
- [ ] **PMKID capture** — Handshake capture initiates correctly
- [ ] **EAPOL capture** — 4-way handshake packets captured
- [ ] **Evil Portal** — AP mode starts, captive portal serves HTML
- [ ] **PCAP save** — Captured packets write to SD/SPIFFS

**Troubleshooting:**
- Deauth fails → Some APs use 802.11w (PMF), expected behavior
- No PCAP → Check SD card mount, verify SPIFFS partition
- Portal not loading → Check AP IP config, DNS redirect

---

## Priority 5: BLE Features

- [ ] **BLE scan starts** — NimBLE initializes without crash
- [ ] **Devices detected** — Nearby BLE devices appear in results
- [ ] **Skimmer detection** — Known skimmer signatures flagged
- [ ] **BLE spam** — Spam packets transmit (verify with second device)
- [ ] **WiFi/BLE coexistence** — Both radios work without crashes

**Troubleshooting:**
- BLE init fail → Verify NimBLE 2 library version, check partition table
- Coexistence crash → May need to disable WiFi before BLE or vice versa
- Low range → Check antenna shared between WiFi/BLE

---

## Priority 6: Storage

- [ ] **SD card detected** — SD card mounts on boot (check serial log)
- [ ] **SD read/write** — Files create and read back correctly
- [ ] **SPIFFS mount** — SPIFFS partition mounts as fallback
- [ ] **SPIFFS read/write** — Files persist across reboots
- [ ] **PCAP files** — Captured packets save to SD card
- [ ] **Settings persist** — Configuration survives power cycle
- [ ] **Evil Portal HTML** — Custom portal pages load from storage

**Troubleshooting:**
- SD not detected → Check SPI pins, try different SD card (FAT32)
- SPIFFS fail → Verify partition table includes SPIFFS partition
- Corrupt files → Check for power loss during write, add fsync

---

## Priority 7: Badge Integration

- [ ] **NeoPixels** — LEDs light up with correct colors
- [ ] **LED state mapping** — Colors change with Marauder state (idle/scan/attack)
- [ ] **Buzzer** — Tones play on events (scan complete, attack start)
- [ ] **Buzzer volume** — Acceptable volume level, not too loud
- [ ] **Battery reading** — ADC returns reasonable voltage
- [ ] **Battery percentage** — Displayed percentage matches actual charge
- [ ] **Badge menu** — Menu renders, navigation works
- [ ] **Marauder launch** — Launching Marauder from badge menu works
- [ ] **Return to menu** — Exiting Marauder returns to badge menu

**Troubleshooting:**
- Wrong LED colors → Check NeoPixel color order (GRB vs RGB)
- No buzzer sound → Verify buzzer pin, check if active or passive buzzer
- Bad battery reading → Calibrate voltage divider ratio in `battery_monitor.h`

---

## Priority 8: Power & OTA

- [ ] **Current draw (active)** — Measure with multimeter, compare to estimates
- [ ] **Current draw (idle)** — Measure idle/menu current
- [ ] **Light sleep** — Device enters sleep, wakes on input
- [ ] **Sleep current** — Measure sleep mode current draw
- [ ] **Battery life** — Estimate runtime from measurements
- [ ] **OTA server** — OTA update endpoint starts on WiFi
- [ ] **OTA flash** — Firmware update completes over WiFi
- [ ] **OTA verify** — Device boots correctly after OTA update

**Troubleshooting:**
- High sleep current → Check peripheral power-down sequence
- OTA fails → Verify partition table has OTA partitions, check flash size
- Won't wake → Verify wake source (touch interrupt, encoder pin)

---

## Final Validation

- [ ] **Extended run** — 1+ hour continuous operation without crash
- [ ] **Memory stability** — Heap doesn't leak over extended use
- [ ] **All features cycle** — Run through every feature sequentially
- [ ] **Power cycle** — Clean boot after hard power off
- [ ] **Multiple badges** — Test on 2+ badges for consistency

---

## Sign-Off

| Test Area | Tester | Date | Pass/Fail | Notes |
|-----------|--------|------|-----------|-------|
| Boot & Display | | | | |
| Touch & Input | | | | |
| WiFi Scanning | | | | |
| WiFi Attacks | | | | |
| BLE Features | | | | |
| Storage | | | | |
| Badge Integration | | | | |
| Power & OTA | | | | |
| Final Validation | | | | |
