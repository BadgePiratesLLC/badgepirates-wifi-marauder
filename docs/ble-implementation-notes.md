# BLE Implementation Notes — BSidesKC Badge

Phase 4 verification of upstream Marauder BLE features (issues #43, #44, #45).

## Configuration

| Setting | Value |
|---------|-------|
| `HAS_BT` | ✅ Defined in `bsideskc_config.h` |
| `HAS_NIMBLE_2` | ❌ Not defined (using NimBLE 1.4.x API) |
| NimBLE-Arduino | v1.4.3 |
| `esp_bt.h` | Included (required for NimBLE 1.x path) |

## BLE Features Present

### Issue #43 — BLE Scanning

All upstream BLE scanning modes compile and are routed through `WiFiScan::RunBluetoothScan()`:

| Scan Mode | Constant | Status |
|-----------|----------|--------|
| General BLE scan | `BT_SCAN_ALL` (10) | ✅ Compiles |
| AirTag sniff | `BT_SCAN_AIRTAG` (43) | ✅ Compiles |
| AirTag monitor | `BT_SCAN_AIRTAG_MON` (70) | ✅ Compiles |
| Flipper sniff | `BT_SCAN_FLIPPER` (45) | ✅ Compiles |
| Flock sniff | `BT_SCAN_FLOCK` (72) | ✅ Compiles |
| Flock wardrive | `BT_SCAN_FLOCK_WARDRIVE` (75) | ✅ Compiles |
| Simple sniff | `BT_SCAN_SIMPLE` (73) | ✅ Compiles |
| Simple sniff 2 | `BT_SCAN_SIMPLE_TWO` (74) | ✅ Compiles |
| BT wardrive | `BT_SCAN_WAR_DRIVE` (34) | ✅ Compiles |
| BT wardrive cont. | `BT_SCAN_WAR_DRIVE_CONT` (35) | ✅ Compiles |
| BT analyzer | `BT_SCAN_ANALYZER` (47) | ✅ Compiles |
| Meta/RayBan detect | `BT_SCAN_RAYBAN` (81) | ✅ Compiles |

### Issue #44 — BLE Skimmer Detection

| Feature | Constant | Status |
|---------|----------|--------|
| Skimmer scan | `BT_SCAN_SKIMMERS` (11) | ✅ Compiles |

Detection algorithm: Matches advertised device names against known skimmer module names (`HC-03`, `HC-05`, `HC-06`). Uses `bluetoothScanAllCallback` with duplicate filtering. Both NimBLE 1.x (`setAdvertisedDeviceCallbacks`) and 2.x (`setScanCallbacks`) code paths are present; badge uses the 1.x path.

### Issue #45 — BLE Spam / Advertising Attacks

All spam/advertising attack modes compile via `WiFiScan::RunSwiftpairSpam()` and `WiFiScan::RunSourApple()`:

| Attack Mode | Constant | Status |
|-------------|----------|--------|
| Sour Apple (Apple popup spam) | `BT_ATTACK_SOUR_APPLE` (36) | ✅ Compiles |
| Swiftpair spam (Microsoft) | `BT_ATTACK_SWIFTPAIR_SPAM` (37) | ✅ Compiles |
| Samsung spam | `BT_ATTACK_SAMSUNG_SPAM` (39) | ✅ Compiles |
| Google Fast Pair spam | `BT_ATTACK_GOOGLE_SPAM` (41) | ✅ Compiles |
| Flipper Zero spam | `BT_ATTACK_FLIPPER_SPAM` (42) | ✅ Compiles |
| Spam all (cycles types) | `BT_ATTACK_SPAM_ALL` (38) | ✅ Compiles |
| AirTag spoof | `BT_SPOOF_AIRTAG` (44) | ✅ Compiles |

Payload generation: `GetUniversalAdvertisementData()` builds manufacturer-specific advertisement payloads for each type (Apple, Microsoft, Samsung, Google, FlipperZero, Airtag). Uses NimBLE 1.x `addData(std::string)` path.

## NimBLE API Compatibility

The upstream code has dual code paths gated by `#ifndef HAS_NIMBLE_2` / `#ifdef HAS_NIMBLE_2`. Since the badge does NOT define `HAS_NIMBLE_2`, all code uses the NimBLE 1.4.x API:

| API Call | NimBLE 1.x (badge) | NimBLE 2.x |
|----------|---------------------|------------|
| Scan callbacks | `setAdvertisedDeviceCallbacks()` | `setScanCallbacks()` |
| Add adv data | `addData(std::string(...))` | `addData(uint8_t*, size_t)` |
| BT memory release | `esp_bt_controller_mem_release()` via `esp_bt.h` | Not used |

All 27 `#ifndef HAS_NIMBLE_2` conditionals in `WiFiScan.cpp` correctly route to the 1.x API. No compatibility issues found.

## Build Verification

```
Platform:  espressif32@6.4.0
Board:     esp32-s3-devkitc-1
NimBLE:    1.4.3
Result:    SUCCESS (0 errors, 0 BLE-related warnings)
RAM:       21.0% (68,960 / 327,680 bytes)
Flash:     22.4% (1,469,481 / 6,553,600 bytes)
```

## Issues Found

None. All BLE features from the upstream Marauder compile cleanly for the ESP32-S3 badge target with NimBLE 1.4.3.

## Notes

- BLE features require runtime testing on hardware to verify NimBLE stack initialization and scan callbacks work correctly on ESP32-S3.
- Wardrive modes (`BT_SCAN_WAR_DRIVE`, `BT_SCAN_FLOCK_WARDRIVE`) depend on `HAS_GPS` for logging; GPS hardware must be present.
- The badge has adequate flash headroom (77.6% free) for all BLE features.
