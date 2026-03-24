# BLE Field Testing — Issues #43–45

> **Only test BLE spam/spoof in controlled environments you own. Do not use in public spaces.**

## Prerequisites

- Target BLE devices nearby (phone, fitness tracker, etc.)
- Serial monitor at 115200 baud
- WiFi stopped before BLE tests (coexistence can be tested separately)

---

## BLE Scan Testing (#43)

**Menu:** Bluetooth → BLE Scan (or CLI: `btscan`)

**Procedure:**
1. Ensure BLE devices are nearby and advertising
2. Navigate to Bluetooth → BLE Scan
3. Verify devices appear on screen with name, address, RSSI
4. Let scan run 30 seconds, check serial for device discovery logs
5. Press BACK to stop
6. Re-scan to verify no stale state

**Expected:** Nearby BLE devices discovered. Names/addresses/RSSI displayed. Clean stop and re-scan.

**Failure signs:** Zero devices found, NimBLE crash, stale results on re-scan, memory leak (check heap).

**Additional scan types to test:**
- `btscan -t airtag` — AirTag detection (need AirTag nearby)
- `btscan -t flipper` — Flipper Zero detection
- `btscan -t meta` — Meta/RayBan smart glasses detection

---

## Skimmer Detection Testing (#44)

**Menu:** Bluetooth → Skimmer Detect (or CLI: `btscan -t skimmer`)

**Procedure:**
1. Navigate to Bluetooth → Skimmer Detect
2. Scan runs and filters for known skimmer BLE signatures
3. Check serial for payload pattern matching logs
4. Verify common consumer devices (phone, watch) are NOT flagged
5. Press BACK to stop

**Expected:** Scan filters advertisements against known skimmer signatures. No false positives on common devices. Alert display if skimmer-like device found.

**Failure signs:** False positives on consumer devices, no filtering applied, crash during scan.

**Note:** Full validation requires a known skimmer device or BLE device spoofing skimmer characteristics. Without one, verify the filtering logic runs without errors and doesn't flag normal devices.

---

## BLE Spam Testing (#45)

**Menu:** Bluetooth → BLE Spam → (select type)

| Type | Target | CLI | What to look for |
|------|--------|-----|-----------------|
| Sour Apple | Apple devices | `blespam -t apple` | Popup notifications on iPhone/iPad |
| SwiftPair | Windows laptops | `blespam -t windows` | Bluetooth pairing popup |
| Samsung | Samsung phones | `blespam -t samsung` | SmartThings/pairing notification |
| Google Fast Pair | Android phones | `blespam -t google` | Fast Pair popup |
| Flipper | Flipper Zero | `blespam -t flipper` | BLE notification on Flipper |
| Spam All | All of above | `blespam -t all` | Multiple device types affected |

**Procedure:**
1. Select a spam type (start with Sour Apple or Google — easiest to verify)
2. Check serial for advertisement construction and TX logs
3. Verify target device shows notification/popup
4. Press BACK to stop
5. Verify advertising stops (no more popups on target)
6. Test WiFi scan after BLE spam to verify coexistence

**Expected:** Target device shows spam notifications. Serial logs advertisement TX. Clean stop. WiFi still works after.

**Failure signs:** No popups on target, BLE stack crash, WiFi broken after BLE spam, crash on repeated start/stop.

---

## Coexistence Testing

After completing individual tests, verify WiFi and BLE work together:

1. Run a WiFi AP scan → stop → run BLE scan → stop
2. Verify both produce results without crash
3. Check heap after each transition (serial output)

---

## Hardware Test Checklist

| # | Test | Pass? | Notes |
|---|------|-------|-------|
| 1 | BLE scan discovers nearby devices | | |
| 2 | Device names and RSSI correct | | |
| 3 | Scan stops cleanly on BACK | | |
| 4 | Re-scan works (no stale state) | | |
| 5 | Skimmer detect runs without false positives | | |
| 6 | BLE spam popups appear on target | | |
| 7 | BLE spam stops cleanly | | |
| 8 | WiFi works after BLE operations | | |
| 9 | No crash on repeated start/stop (3× each) | | |
| 10 | Memory stable across BLE tests | | |
