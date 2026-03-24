# Field Testing Guide — BSidesKC WiFi Marauder Badge

> **This guide covers hands-on hardware testing of WiFi and BLE features
> that cannot be validated through compilation alone.**

---

## ⚠️ Legal & Safety Warnings

### Legal Requirements

- **Only test on networks and devices you own or have explicit written authorization to test**
- Unauthorized use of deauth, beacon spam, Evil Portal, or BLE spam features is **illegal** in most jurisdictions
- Applicable laws include (but are not limited to):
  - **US:** Computer Fraud and Abuse Act (CFAA), FCC Part 15 regulations
  - **UK:** Computer Misuse Act 1990
  - **EU:** Directive 2013/40/EU on attacks against information systems
- PMKID/EAPOL capture must only target your own access points
- Evil Portal must not be used to collect credentials from unsuspecting users
- BLE spam in public spaces affects all nearby devices — test in isolation only

### Safety Precautions

- Use an **RF-shielded environment or Faraday cage** when possible
- Use a **dedicated test AP** isolated from production networks (no internet uplink)
- Use **non-overlapping WiFi channels** to minimize interference with neighbors
- **Power down attack features immediately** after testing
- Keep a **written log** of all testing activities with timestamps
- Have a **second monitoring device** (laptop with Wireshark, phone) to verify TX behavior

### Disclaimer

This firmware is a port of the open-source ESP32Marauder security research tool.
All features are intended for **authorized security research and education only**.
The developers assume no liability for misuse.

---

## Test Environment Setup

### Required Equipment

| Item | Purpose |
|------|---------|
| BSidesKC badge (flashed with Marauder firmware) | Device under test |
| FAT32 micro SD card (≥4 GB) | PCAP saves, Evil Portal HTML, settings |
| USB-C cable + power source | Power and serial monitor |
| Laptop with serial terminal (115200 baud) | Debug output monitoring |
| Dedicated test WiFi AP (WPA2) | WiFi attack target |
| Test client device (phone/laptop) | Verify deauth, Evil Portal, BLE spam |
| Second WiFi adapter in monitor mode (optional) | Verify raw frame TX |
| USB power meter (optional) | Current draw measurement |

### Firmware Preparation

```bash
# Build and flash
pio run -e bsideskc-badge-production -t upload

# Monitor serial output
pio device monitor -b 115200
```

### SD Card Preparation

Format as FAT32, then create:
```
/SCRIPTS/          (auto-created by firmware)
/index.html        (Evil Portal test page)
/ap.config.txt     (optional: custom AP SSID for Evil Portal)
```

Evil Portal test HTML:
```html
<html><body><h1>Test Portal</h1>
<form method="POST" action="/post">
<input name="user" placeholder="Username">
<input name="pass" type="password" placeholder="Password">
<button>Login</button>
</form></body></html>
```

### Serial Monitor Setup

All test events are logged to serial at 115200 baud with prefixed tags:
- `[WiFiTest]` — WiFi scan test mode events
- `[LED]` — NeoPixel state changes
- `[SD]` — SD card operations
- `[BUF/BEGIN]` / `[BUF/CLOSE]` — PCAP buffer operations
- `[Battery]` — Battery monitor events
- `[Settings]` — Settings load/save

---

## WiFi Attack Testing (Issues #38–41)

Detailed procedures for each WiFi attack feature are in **[wifi-attack-testing.md](wifi-attack-testing.md)**.

### Quick Reference

| Feature | Issue | Menu Path | Key Verification |
|---------|-------|-----------|-----------------|
| Deauth | #38 | WiFi → Scan APs → Select → Deauth | Target clients disconnect |
| Beacon Spam | #39 | WiFi → Beacon Spam → Random | Fake SSIDs visible on phone |
| PMKID Capture | #40 | WiFi → Scan APs → Select → PMKID Scan | PCAP valid in Wireshark |
| Evil Portal | #41 | WiFi → Evil Portal | Captive portal loads on client |

### Pre-Test Checklist (WiFi)

- [ ] Badge powered on, Marauder menu visible
- [ ] SD card inserted and mounted (check serial for `[SD] Card Type: SDHC`)
- [ ] Test AP powered on and broadcasting
- [ ] Test client device connected to test AP
- [ ] Serial monitor running at 115200 baud
- [ ] No production networks in range (or using Faraday cage)

---

## BLE Testing

Detailed procedures for each BLE feature are in **[ble-testing.md](ble-testing.md)**.

### Quick Reference

| Feature | Issue | Menu Path | Key Verification |
|---------|-------|-----------|-----------------|
| BLE Scan | #43 | Bluetooth → BLE Scan | Nearby devices listed |
| Skimmer Detect | #44 | Bluetooth → Skimmer Detect | Signature matching works |
| BLE Spam | #45 | Bluetooth → BLE Spam → (type) | Popups on target device |

### Pre-Test Checklist (BLE)

- [ ] Badge powered on, Marauder menu visible
- [ ] Target BLE devices nearby (phone, fitness tracker, etc.)
- [ ] Serial monitor running
- [ ] No public BLE devices in range that could be affected

---

## Expected Results Summary

### WiFi Features

| Test | Expected Outcome | Failure Indicator |
|------|-----------------|-------------------|
| AP Scan | Discovers all nearby APs with SSID/RSSI/channel | Zero APs found, crash |
| Deauth | Target clients disconnect within 5 seconds | No disconnection, `esp_wifi_80211_tx` error in serial |
| Beacon Spam | 10+ fake SSIDs visible on phone within 3 seconds | No SSIDs appear, serial shows frame errors |
| PMKID Capture | PCAP file created, valid in Wireshark | No PCAP, empty file, Wireshark parse error |
| Evil Portal | Captive portal auto-opens on client | AP not visible, DNS redirect fails, blank page |

### BLE Features

| Test | Expected Outcome | Failure Indicator |
|------|-----------------|-------------------|
| BLE Scan | Discovers nearby BLE devices with name/RSSI | Zero devices, NimBLE crash |
| Skimmer Detect | Filters and flags suspicious devices | False positives on common devices, no filtering |
| BLE Spam | Target device shows notification popups | No popups, BLE stack crash |

---

## Troubleshooting

### WiFi Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No APs found in scan | WiFi not initialized | Check serial for `esp_wifi_start` errors; power cycle |
| Deauth has no effect | Raw TX not working on S3 | Check serial for `esp_wifi_80211_tx` return code; try different target |
| Beacon spam SSIDs not visible | Frame rate too low or TX failure | Monitor serial for frame construction logs; check channel |
| PMKID scan captures nothing | No client reconnection occurred | Force client reconnect; ensure target is WPA2 |
| Evil Portal page blank | HTML not loaded from SD | Check SD mount; try `sethtml=` via serial as fallback |
| Crash during WiFi operation | Memory exhaustion | Check free heap in serial; reduce buffer sizes |
| Watchdog reset during scan | Task blocking too long | Check for infinite loops in serial output |

### BLE Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| BLE scan finds nothing | NimBLE not initialized | Check serial for BLE init errors; verify `HAS_BT` enabled |
| Crash on BLE scan start | WiFi/BLE coexistence conflict | Stop WiFi before starting BLE; check memory |
| BLE spam no effect | Advertisements not transmitting | Check serial for adv construction logs; try different spam type |
| Repeated scan crash | Memory leak in NimBLE | Monitor heap between scans; power cycle between tests |

### General Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| SD card not detected | SPI bus conflict or bad card | Try different card; check serial for SPI errors |
| Settings not persisting | SPIFFS corruption | Erase SPIFFS partition and reflash |
| Display garbled after attack | SPI bus contention | Stop attack, power cycle |
| Badge unresponsive | Watchdog or hard fault | Hold BOOT for 10s to force reset; reflash |

---

## Test Result Recording

Use this template for each test session:

```
Date: YYYY-MM-DD
Tester: 
Firmware version: 
Environment: [lab/faraday cage/outdoor]

WiFi Tests:
  AP Scan:        [PASS/FAIL] — notes
  Deauth (#38):   [PASS/FAIL] — notes
  Beacon (#39):   [PASS/FAIL] — notes
  PMKID (#40):    [PASS/FAIL] — notes
  Evil Portal (#41): [PASS/FAIL] — notes

BLE Tests:
  BLE Scan (#43):     [PASS/FAIL] — notes
  Skimmer (#44):      [PASS/FAIL] — notes
  BLE Spam (#45):     [PASS/FAIL] — notes

Issues Found:
  1. 
  2. 
```
