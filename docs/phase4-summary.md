# Phase 4 Summary: BLE Features

## Overview

Phase 4 verified that all upstream ESP32Marauder BLE features compile successfully for the BSidesKC ESP32-S3 badge using NimBLE-Arduino 1.4.3. No source modifications were required.

## BLE Features Verified (Compile-Verified)

### Scanning
| Feature | Constant | Status |
|---------|----------|--------|
| BLE Scan All | `BT_SCAN_ALL` | ✅ Compiles |
| Skimmer Scan | `BT_SCAN_SKIMMERS` | ✅ Compiles |
| AirTag Scan | `BT_SCAN_AIRTAG` | ✅ Compiles |
| AirTag Monitor | `BT_SCAN_AIRTAG_MON` | ✅ Compiles |
| Flipper Scan | `BT_SCAN_FLIPPER` | ✅ Compiles |
| Flock Scan | `BT_SCAN_FLOCK` | ✅ Compiles |
| Meta/RayBan Scan | `BT_SCAN_RAYBAN` | ✅ Compiles |
| BLE Analyzer | `BT_SCAN_ANALYZER` | ✅ Compiles |
| Simple Scan | `BT_SCAN_SIMPLE` | ✅ Compiles |
| BT War Drive | `BT_SCAN_WAR_DRIVE` | ✅ Compiles |

### Attacks/Spam
| Feature | Constant | Status |
|---------|----------|--------|
| Sour Apple | `BT_ATTACK_SOUR_APPLE` | ✅ Compiles |
| SwiftPair Spam | `BT_ATTACK_SWIFTPAIR_SPAM` | ✅ Compiles |
| Samsung Spam | `BT_ATTACK_SAMSUNG_SPAM` | ✅ Compiles |
| Google Spam | `BT_ATTACK_GOOGLE_SPAM` | ✅ Compiles |
| Flipper Spam | `BT_ATTACK_FLIPPER_SPAM` | ✅ Compiles |
| Spam All | `BT_ATTACK_SPAM_ALL` | ✅ Compiles |

### Spoofing
| Feature | Constant | Status |
|---------|----------|--------|
| AirTag Spoof | `BT_SPOOF_AIRTAG` | ✅ Compiles |

## NimBLE 1.4.3 Compatibility

- **Version**: NimBLE-Arduino 1.4.3 (`h2zero/NimBLE-Arduino@^1.4.0`)
- **ESP32-S3 support**: Fully supported
- **API path**: NimBLE 1.x (`HAS_NIMBLE_2` not defined, matching upstream V8 config)
- **NimBLE 2.x upgrade**: Not needed — upstream V8 board also uses NimBLE 1.x
- **`CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE`**: NimBLE 1.4.3 provides fallback define for ESP32-S3 (value `0` = filter by device address)
- **`esp_bt.h`**: Available via ESP32-S3 SDK `esp32c3/include/esp_bt.h`
- **`esp_ble_gap_set_rand_addr`**: Available in ESP32-S3 Bluedroid API

## ESP32-S3 BLE 5.0 Support

The ESP32-S3 provides BLE 5.0 (BLE-only, no Classic Bluetooth):
- **2M PHY**: Higher throughput available
- **Coded PHY / Long Range**: Extended range mode
- **Extended Advertising**: Up to 1650 bytes payload
- **Multiple Advertising Sets**: Concurrent advertisements
- **Periodic Advertising**: Scheduled broadcasts

BLE 5.0 extended features are available but not used by upstream Marauder — potential future enhancement.

## Compilation Status

```
Platform: espressif32@6.4.0
Board:    esp32-s3-devkitc-1
NimBLE:   1.4.3
Build:    SUCCESS (zero errors, zero warnings related to BLE)
RAM:      21.0% (68960 / 327680 bytes)
Flash:    22.4% (1469481 / 6553600 bytes)
```

All BLE code compiles via the NimBLE 1.x code path with `HAS_BT` enabled and `HAS_NIMBLE_2` omitted. Zero upstream source modifications required.

## What Requires Hardware Testing

The following cannot be verified without physical badge hardware:

1. **BLE Scan All** — Device discovery, RSSI accuracy, scan duration
2. **Skimmer Detection** — Payload pattern matching against known skimmer signatures
3. **AirTag Detection** — Apple continuity protocol parsing
4. **BLE Spam Attacks** — Advertisement transmission, target device response
5. **AirTag Spoofing** — Spoofed advertisement acceptance by Apple devices
6. **Flipper/Flock Detection** — Device signature matching
7. **BLE + WiFi Coexistence** — Simultaneous radio usage on ESP32-S3
8. **Memory Under BLE Load** — Heap usage during active scanning with many devices

## Known Limitations

1. **No Classic Bluetooth**: ESP32-S3 is BLE-only. This is not an issue since Marauder uses BLE exclusively.
2. **BLE 5.0 features unused**: Upstream Marauder doesn't leverage 2M PHY, Coded PHY, or extended advertising. Could improve scan range/speed in future.
3. **War Drive requires GPS**: `BT_SCAN_WAR_DRIVE` needs a GPS module (not present on badge). Feature compiles but will have no GPS data.
4. **No runtime validation**: All features are compile-verified only. Hardware testing deferred to when badges are available.
5. **Coexistence untested**: WiFi + BLE simultaneous operation on ESP32-S3 needs hardware validation.

## Issues Addressed

- **#42** — Configure NimBLE for ESP32-S3: NimBLE 1.4.3 works without changes
- **#43** — Test BLE scanning: Compile-verified, hardware test pending
- **#44** — Test BLE skimmer detection: Compile-verified, hardware test pending
- **#45** — Test BLE spam attacks: Compile-verified, hardware test pending

## Phase 5 Preview

Phase 5 (Storage & Persistence) will cover:
- SD card SPI interface configuration (#46)
- PCAP file saves to SD (#47)
- Evil Portal HTML storage on SD (#48)
- Settings persistence (#49)
- SPIFFS fallback storage (#50)
