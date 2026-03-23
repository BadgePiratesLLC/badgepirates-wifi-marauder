# WiFi Implementation Notes — ESP32-S3 Compatibility

## Phase 3 — Issue #35: Verify Promiscuous Mode on ESP32-S3

### Summary

**ESP32-S3 fully supports WiFi promiscuous mode.** This is NOT a blocker.

The upstream ESP32 Marauder WiFi stack compiles and links cleanly for
ESP32-S3 (target: `esp32-s3-devkitc-1`) with zero WiFi-related errors.
Build verified on `espressif32@6.4.0` / Arduino ESP32 2.0.11 (ESP-IDF 4.4.x).

---

## ESP32-S3 WiFi Capabilities

| Feature | ESP32 Classic | ESP32-S3 | Status |
|---|---|---|---|
| 802.11 b/g/n (2.4 GHz) | ✅ | ✅ | Identical |
| Promiscuous / Sniffer mode | ✅ | ✅ | Fully supported |
| `esp_wifi_set_promiscuous()` | ✅ | ✅ | Same API |
| `esp_wifi_set_promiscuous_filter()` | ✅ | ✅ | Same API |
| `esp_wifi_set_promiscuous_rx_cb()` | ✅ | ✅ | Same API |
| `esp_wifi_80211_tx()` (raw TX) | ✅ | ✅ | Same API |
| `esp_wifi_set_channel()` | ✅ | ✅ | Same API |
| STA + AP + Sniffer coexistence | ✅ | ✅ | Same behavior |
| 5 GHz / Dual-band | ❌ | ❌ | Neither supports 5 GHz natively |
| WPA3-SAE | ✅ | ✅ | Both supported |
| CSI (Channel State Information) | ✅ | ✅ | Both supported |
| PMF (Protected Mgmt Frames) | ✅ | ✅ | Both supported |

### Key Finding: No WiFi API Differences

The ESP-IDF WiFi driver exposes an identical API surface for ESP32 and
ESP32-S3. The `esp_wifi.h` header, promiscuous mode callbacks, filter
types (`wifi_promiscuous_filter_t`, `WIFI_PROMIS_FILTER_MASK_MGMT`,
`WIFI_PROMIS_FILTER_MASK_DATA`), and raw TX function are all present
and functional on ESP32-S3.

---

## Promiscuous Mode — Detailed Analysis

### How Upstream Marauder Uses Promiscuous Mode

The upstream `WiFiScan.cpp` uses promiscuous mode extensively:

1. **`setWiFiMode()`** — Central setup function that calls:
   - `esp_wifi_set_promiscuous(true)`
   - `esp_wifi_set_promiscuous_filter(&filt)` (MGMT + DATA frames)
   - `esp_wifi_set_promiscuous_rx_cb(cb)` with various callbacks

2. **Sniffer callbacks** (all static, all compatible with S3):
   - `beaconSnifferCallback` — AP scanning, beacon capture
   - `wifiSnifferCallback` — General packet monitor
   - `eapolSnifferCallback` — EAPOL/handshake capture
   - `apSnifferCallbackFull` — Full AP enumeration
   - `pineScanSnifferCallback` — Pineapple detection
   - `multiSSIDSnifferCallback` — Multi-SSID detection

3. **Filter configuration** (defined in WiFiScan.h):
   ```cpp
   const wifi_promiscuous_filter_t filt = {
       .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
   };
   ```

4. **Raw packet injection** via `esp_wifi_80211_tx()`:
   - Beacon spam, deauth frames, probe requests
   - EAPOL bad-message attacks, SAE commit frames
   - All use `WIFI_IF_AP` or `WIFI_IF_STA` interface

### ESP32-S3 Promiscuous Mode Confirmation

Per Espressif's official ESP-IDF documentation for ESP32-S3:

> "Promiscuous mode for monitoring of IEEE802.11 Wi-Fi packets."
> — ESP-IDF v5.5.3, esp_wifi API Reference (ESP32-S3 target)

Sniffer mode can dump:
- 802.11 Management frames
- 802.11 Data frames (MPDU, AMPDU, AMSDU)
- 802.11 MIMO frames (length only)
- 802.11 Control frames
- 802.11 CRC error frames

Can be enabled in: `WIFI_MODE_NULL`, `WIFI_MODE_STA`, `WIFI_MODE_AP`, `WIFI_MODE_APSTA`

### `esp_wifi_80211_tx()` on ESP32-S3

Per Espressif docs (ESP32-S3 target):
- Can send beacon, probe request, probe response, action frames
- Can send non-QoS data frames
- Cannot send encrypted or QoS frames
- Preconditions: WiFi mode set + either `esp_wifi_set_promiscuous(true)` or `esp_wifi_start()` called

This is identical behavior to ESP32 classic.

---

## Build Verification

### Compilation Test Results

```
Platform:  espressif32@6.4.0
Board:     esp32-s3-devkitc-1
Framework: Arduino (ESP32 2.0.11, ESP-IDF 4.4.x)
Toolchain: xtensa-esp32s3 8.4.0

Result:    SUCCESS
RAM:       21.0% (68944 / 327680 bytes)
Flash:     22.4% (1467909 / 6553600 bytes)
```

WiFiScan.cpp compiles with zero errors and zero WiFi-related warnings
for the ESP32-S3 target.

### Why It Works — `HAS_IDF_3` Flag

The badge config defines `HAS_IDF_3`, which activates the correct code
paths in upstream Marauder:

1. **`cfg2` initialization** — Uses `WIFI_INIT_CONFIG_DEFAULT()` macro
   (safe, portable) instead of manually specifying internal ESP-IDF
   struct fields (`g_wifi_osi_funcs`, `g_wifi_feature_caps`, etc.)

2. **Network interface** — Uses `esp_netif.h` / `esp_mac.h` includes
   instead of legacy `lwip/etharp.h` paths

3. **WiFi country config** — Uses `wifi_country_t` struct for channel
   configuration

---

## Known Limitations & Differences

### 1. Single-Band Only (2.4 GHz)
The badge ESP32-S3 is single-band. `HAS_DUAL_BAND` is intentionally
NOT defined. Channel hopping is limited to channels 1-14. The upstream
code handles this correctly via `#ifndef HAS_DUAL_BAND` guards.

### 2. No NimBLE 2.x
Badge uses NimBLE 1.4.x (`HAS_NIMBLE_2` not defined). This affects BT
scanning but not WiFi features.

### 3. XIAO_ESP32_S3 Code Paths
Upstream has `XIAO_ESP32_S3` guards for LED control on Seeed XIAO S3
boards. Our badge does NOT use these — we use `HAS_NEOPIXEL_LED` via
`LedInterface` instead. No conflict.

### 4. Memory Considerations
ESP32-S3 has 512KB SRAM vs ESP32's 520KB. With PSRAM enabled (badge
has PSRAM), the `mac_history` buffer is heap-allocated. Current build
uses 21% RAM — plenty of headroom for WiFi operations.

### 5. ESP-IDF Version
Badge uses ESP-IDF 4.4.x (via Arduino ESP32 2.0.11). All promiscuous
mode APIs are available in this version. Upgrading to ESP-IDF 5.x
(Arduino ESP32 3.x) would require testing but should not break WiFi
functionality.

---

## Required Changes for S3 Compatibility

**None required for core WiFi functionality.** The existing configuration
(`HAS_IDF_3`, `MARAUDER_V8` base, no `HAS_DUAL_BAND`) already provides
correct ESP32-S3 compatibility for:

- WiFi scanning (AP discovery)
- Promiscuous mode packet capture
- Raw packet injection
- Channel hopping (2.4 GHz)
- EAPOL capture
- Deauth/beacon attacks
- Evil Portal
- Packet monitoring

---

## Phase 3 Roadmap

- [x] Verify promiscuous mode support (Issue #35) — **CONFIRMED**
- [x] Verify WiFiScan.cpp compiles for ESP32-S3 — **CONFIRMED**
- [ ] Runtime test: AP scanning on hardware
- [ ] Runtime test: Promiscuous mode packet capture
- [ ] Runtime test: Channel hopping
- [ ] Runtime test: Raw packet injection
- [ ] Runtime test: PCAP file writing to SD
