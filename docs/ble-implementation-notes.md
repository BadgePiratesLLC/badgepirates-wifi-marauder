# Phase 4: BLE Implementation Notes

## ESP32-S3 BLE Capabilities

- **BLE Version**: BLE 5.0 (Bluetooth LE only — no Classic BT)
- **Controller**: ESP32-S3 uses a BLE-only controller (`esp32c3`-style), not the BTDM dual-mode controller of the original ESP32
- **BLE 5.0 Features Available**:
  - 2M PHY (higher throughput)
  - Coded PHY / Long Range
  - Extended advertising (up to 1650 bytes payload)
  - Multiple advertising sets
  - Periodic advertising
- **Key Difference from ESP32**: No Classic Bluetooth (BR/EDR) — BLE only. This is fine for Marauder since all BT features use BLE.

## NimBLE Version Compatibility

| Item | Status |
|------|--------|
| **Current version** | NimBLE-Arduino 1.4.3 (`h2zero/NimBLE-Arduino@^1.4.0`) |
| **ESP32-S3 support** | ✅ Fully supported in 1.4.x |
| **Build status** | ✅ Compiles successfully for ESP32-S3 |
| **HAS_NIMBLE_2 flag** | Not defined (correct for NimBLE 1.4.x) |
| **NimBLE 2.x upgrade needed?** | No — upstream V8 board also uses NimBLE 1.x (`HAS_NIMBLE_2` commented out) |

### NimBLE 1.4.x vs 2.x API Differences

Upstream Marauder has dual-path code gated by `#ifdef HAS_NIMBLE_2`:
- **NimBLE 1.x** (`!HAS_NIMBLE_2`): `NimBLEAdvertisedDeviceCallbacks` with `void onResult(NimBLEAdvertisedDevice*)`, `getPayload()` returns `uint8_t*`, `getPayloadLength()` for size
- **NimBLE 2.x** (`HAS_NIMBLE_2`): `NimBLEScanCallbacks` with `void onResult(const NimBLEAdvertisedDevice*)`, `getPayload()` returns `const std::vector<unsigned char>&`

Our badge uses the NimBLE 1.x code path, matching the upstream V8 configuration.

## ESP32-S3 Specific Considerations

### CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE

- ESP32 classic defines `CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE` in its SDK
- ESP32-S3 defines `CONFIG_BT_CTRL_SCAN_DUPL_TYPE_DEVICE` instead
- **Resolution**: NimBLE 1.4.3 `nimconfig.h` provides a fallback: `#define CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE 0` when not defined. Value `0` = filter by device address (correct behavior).
- No code changes needed.

### esp_bt.h Include

- `WiFiScan.h` includes `esp_bt.h` when `HAS_BT && !HAS_NIMBLE_2` (our case)
- ESP32-S3 SDK provides `esp_bt.h` via `esp32c3/include/esp_bt.h`
- ✅ Compiles without issues

### esp_ble_gap_set_rand_addr

- Used in `WiFiScan.cpp` via `extern "C"` declaration
- Available in ESP32-S3 SDK at `bt/host/bluedroid/api/include/api/esp_gap_ble_api.h`
- ✅ Compatible

## Upstream BLE Features Found

### Scanning Features
| Feature | Scan Mode Constant | Description |
|---------|-------------------|-------------|
| BLE Scan All | `BT_SCAN_ALL` | General BLE device discovery |
| Skimmer Scan | `BT_SCAN_SKIMMERS` | Detect BLE credit card skimmers |
| AirTag Scan | `BT_SCAN_AIRTAG` | Detect Apple AirTags |
| AirTag Monitor | `BT_SCAN_AIRTAG_MON` | Continuous AirTag monitoring |
| Flipper Scan | `BT_SCAN_FLIPPER` | Detect Flipper Zero devices |
| Flock Scan | `BT_SCAN_FLOCK` | Flock detection |
| Meta/RayBan Scan | `BT_SCAN_RAYBAN` | Detect Meta smart glasses |
| BLE Analyzer | `BT_SCAN_ANALYZER` | BLE signal analysis |
| Simple Scan | `BT_SCAN_SIMPLE` / `BT_SCAN_SIMPLE_TWO` | Simplified BLE scan |
| BT War Drive | `BT_SCAN_WAR_DRIVE` / `BT_SCAN_WAR_DRIVE_CONT` | BLE wardriving with GPS |
| Flock Wardrive | `BT_SCAN_FLOCK_WARDRIVE` | Flock wardrive mode |

### Attack/Spam Features
| Feature | Scan Mode Constant | Description |
|---------|-------------------|-------------|
| Sour Apple | `BT_ATTACK_SOUR_APPLE` | Apple BLE popup spam |
| SwiftPair Spam | `BT_ATTACK_SWIFTPAIR_SPAM` | Windows SwiftPair spam |
| Samsung Spam | `BT_ATTACK_SAMSUNG_SPAM` | Samsung BLE spam |
| Google Spam | `BT_ATTACK_GOOGLE_SPAM` | Google Fast Pair spam |
| Flipper Spam | `BT_ATTACK_FLIPPER_SPAM` | Flipper Zero BLE spam |
| Spam All | `BT_ATTACK_SPAM_ALL` | All BLE spam types |

### Spoofing Features
| Feature | Scan Mode Constant | Description |
|---------|-------------------|-------------|
| AirTag Spoof | `BT_SPOOF_AIRTAG` | Spoof Apple AirTag |

### CLI Commands
- `btscan [-t <type>]` — BLE scanning (types: airtag, flipper, flock, meta, general)
- `blespam -t <apple/google/samsung/windows/flipper/all>` — BLE spam attacks
- `spoofat` — AirTag spoofing

## Build Verification (Phase 4 Issue #42)

```
Platform: espressif32@6.4.0
Board: esp32-s3-devkitc-1
NimBLE: 1.4.3
Build: SUCCESS
RAM:   21.0% (68960 / 327680 bytes)
Flash: 22.4% (1469481 / 6553600 bytes)
```

All upstream BLE code compiles for ESP32-S3 without modifications. The `HAS_BT` flag is enabled in `bsideskc_config.h`, and `HAS_NIMBLE_2` is correctly omitted.

## Recommendations

1. **No NimBLE upgrade needed** — 1.4.3 works correctly with ESP32-S3 and matches upstream V8 config
2. **BLE 5.0 extended features** (2M PHY, extended advertising) are available but not used by upstream Marauder — could be a future enhancement
3. **Runtime testing required** — compilation success doesn't guarantee all BLE features work correctly on S3 hardware. Priority test targets:
   - BLE scan all (basic device discovery)
   - BLE spam attacks (advertising)
   - AirTag detection (payload parsing)
4. **No S3-specific code changes needed** for BLE compilation
