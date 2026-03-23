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

## Phase 3 — Issue #37: Packet Monitoring

### How Promiscuous Mode Is Used

Upstream Marauder's packet monitoring is driven by `WiFiScan::RunPacketMonitor()`:

1. **LED mode** set to `MODE_SNIFF` via `setLEDMode()`
2. **PCAP file** opened via `startPcap("packet_monitor")` if `SavePCAP` is enabled
3. **Display** initialized (graph objects, color key, scale buttons on ILI9341)
4. **WiFi driver** initialized:
   ```cpp
   esp_wifi_init(&cfg2);              // WIFI_INIT_CONFIG_DEFAULT (HAS_IDF_3)
   esp_wifi_set_country(&country);
   esp_event_loop_create_default();
   setWiFiMode(WIFI_MODE_NULL, wifiSnifferCallback);
   ```
5. **`setWiFiMode()`** is the central promiscuous setup:
   ```cpp
   esp_wifi_set_storage(WIFI_STORAGE_RAM);
   esp_wifi_set_mode(mode);           // WIFI_MODE_NULL for monitoring
   esp_wifi_start();
   esp_wifi_set_promiscuous(true);
   esp_wifi_set_promiscuous_filter(&filt);
   esp_wifi_set_promiscuous_rx_cb(cb);  // wifiSnifferCallback
   ```
6. **Channel** set via `changeChannel(set_channel)`

### Packet Types Captured

The promiscuous filter is defined in `WiFiScan.h`:
```cpp
const wifi_promiscuous_filter_t filt = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
};
```

| Frame Type | Filter Mask | What's Captured |
|---|---|---|
| Management | `WIFI_PROMIS_FILTER_MASK_MGMT` | Beacons (0x80), Deauths (0xA0/0xC0), Probes (0x40) |
| Data | `WIFI_PROMIS_FILTER_MASK_DATA` | All 802.11 data frames |
| Control | Not in filter | Not captured by default |

### Callback: `wifiSnifferCallback`

The static callback receives every frame matching the filter:

```cpp
void WiFiScan::wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
```

Processing flow:
1. Cast `buf` to `wifi_promiscuous_pkt_t*`
2. Extract `WifiMgmtHdr` from payload for frame control parsing
3. Read `rx_ctrl` for signal/channel metadata
4. **MGMT frames**: classify by subtype byte (`payload[0]`):
   - `0x80` → beacon (green, `num_beacon++`)
   - `0xA0`/`0xC0` → deauth/disassoc (red, `num_deauth++`)
   - `0x40` → probe request (cyan, `num_probe++`)
   - Other → magenta
5. **DATA frames**: displayed in white
6. Extract src/dst MAC via `getMAC()` at payload offsets 10 and 4
7. Format display string: `"src_addr -> dst_addr"`
8. Push to `display_buffer` for screen rendering
9. Call `buffer_obj.append(snifferPacket, len)` for PCAP capture

### PCAP Capture Integration

When `SavePCAP` setting is enabled:
- `startPcap("packet_monitor")` opens a `.pcap` file on SD via `Buffer::pcapOpen()`
- `Buffer::append()` checks the setting, then calls `add(payload, len, true)`
- `add()` writes PCAP record header (ts_sec, ts_usec, incl_len, orig_len) + payload
- Double-buffered (A/B) with `BUF_SIZE` = 8KB, `SNAP_LEN` = 4096

### ESP32-S3 Compatibility — Verified

All packet monitoring code compiles cleanly for ESP32-S3:

- **Callback signature**: `void(*)(void*, wifi_promiscuous_pkt_type_t)` — identical on S3
- **Filter masks**: `WIFI_PROMIS_FILTER_MASK_MGMT`, `WIFI_PROMIS_FILTER_MASK_DATA` — present in ESP-IDF 4.4.x for S3
- **Packet structures**: `wifi_promiscuous_pkt_t`, `wifi_pkt_rx_ctrl_t` — same layout on S3
- **`WIFI_MODE_NULL`**: Supported on S3 for promiscuous-only operation
- **Build result**: SUCCESS — 21.0% RAM, 22.4% Flash (no increase from baseline)

### Performance Considerations

1. **Callback runs in WiFi task context** — must be fast, no blocking calls
2. **String allocation in callback**: upstream builds display strings with `String::concat()` — heap pressure under high packet rates
3. **Display buffer limit**: `SCREEN_BUFFER` caps at `MAX_SCREEN_BUFFER` (21) entries — overflow packets are dropped from display but still captured to PCAP
4. **Channel hopping**: packet monitor runs on a single channel; user selects channel via UI. No automatic hopping during monitoring
5. **Memory**: with PSRAM enabled and 21% RAM usage, headroom is sufficient for sustained capture
6. **SD write latency**: double-buffered PCAP writes minimize callback blocking, but SD card speed can cause drops at very high packet rates

---

## Phase 3 Roadmap

- [x] Verify promiscuous mode support (Issue #35) — **CONFIRMED**
- [x] Verify WiFiScan.cpp compiles for ESP32-S3 — **CONFIRMED**
- [x] WiFi scan test mode added (ENTER+BACK boot combo)
- [x] Packet monitoring verified for ESP32-S3 (Issue #37) — **CONFIRMED**
- [ ] Runtime test: AP scanning on hardware
- [ ] Runtime test: Promiscuous mode packet capture
- [ ] Runtime test: Channel hopping
- [ ] Runtime test: Raw packet injection
- [ ] Runtime test: PCAP file writing to SD

## Phase 3 — Issues #38-41: Upstream WiFi Feature Compilation Verification

> **Note:** This is a port of the existing open-source ESP32Marauder security
> research tool. All WiFi features below are upstream implementations — we are
> verifying they compile for ESP32-S3, not creating new attack tools.

### Build Result

```
Platform:  espressif32@6.4.0
Board:     esp32-s3-devkitc-1
Framework: Arduino (ESP32 2.0.11, ESP-IDF 4.4.x)
Toolchain: xtensa-esp32s3 8.4.0

Result:    SUCCESS — zero errors, zero WiFi-related warnings
RAM:       21.0% (68960 / 327680 bytes)
Flash:     22.4% (1469481 / 6553600 bytes)
```

### Feature Compilation Status

| Issue | Feature | Upstream Functions | `esp_wifi_80211_tx` | Linked in Binary | Status |
|-------|---------|-------------------|---------------------|------------------|--------|
| #38 | Deauth | `RunDeauthScan()`, `sendDeauthFrame()` | ✅ Used at lines 8896-8898, 8951 | ✅ `42035910 T` | **COMPILES** |
| #39 | Beacon Spam | `RunBeaconScan()` (spam + list modes) | ✅ Used at lines 8797, 8849 | ✅ `420354dc T` | **COMPILES** |
| #40 | PMKID/EAPOL Capture | `RunEapolScan()`, `eapolSnifferCallback()` | N/A (passive capture) | ✅ `42034da0 T` | **COMPILES** |
| #41 | Evil Portal | `RunEvilPortal()`, `EvilPortal::begin/setup/main` | N/A (web server) | ✅ `420341d0 T` | **COMPILES** |

### Issue #38 — Deauth (Compilation Verified)

Upstream deauth implementation in `WiFiScan.cpp`:
- **Scan modes:** `WIFI_SCAN_DEAUTH` (5), `WIFI_ATTACK_DEAUTH` (20), `WIFI_ATTACK_DEAUTH_MANUAL` (24), `WIFI_ATTACK_DEAUTH_TARGETED` (27)
- **Entry point:** `WiFiScan::RunDeauthScan()` (line 5250)
- **Frame injection:** `WiFiScan::sendDeauthFrame()` (line 9017) uses `esp_wifi_80211_tx(WIFI_IF_AP, ...)`
- **Frame template:** `deauth_frame_default[26]` defined in `WiFiScan.h` (line 470)
- **ESP32-S3 status:** `esp_wifi_80211_tx` symbol present at `0x420c94d4` — identical API to ESP32

### Issue #39 — Beacon Spam (Compilation Verified)

Upstream beacon spam implementation in `WiFiScan.cpp`:
- **Scan modes:** `WIFI_ATTACK_BEACON_SPAM` (8), `WIFI_ATTACK_BEACON_LIST` (15), `WIFI_ATTACK_FUNNY_BEACON` (99)
- **Entry point:** `WiFiScan::RunBeaconScan()` (line 5054)
- **Frame injection:** `esp_wifi_80211_tx(WIFI_IF_AP, temp_frame, ...)` at lines 8797, 8849
- **Beacon sniffer:** `beaconSnifferCallback()` for AP discovery
- **ESP32-S3 status:** All beacon frame construction and injection compiles cleanly

### Issue #40 — PMKID/EAPOL Capture (Compilation Verified)

Upstream EAPOL/PMKID capture implementation in `WiFiScan.cpp`:
- **Scan modes:** `WIFI_SCAN_EAPOL` (4), `WIFI_SCAN_ACTIVE_EAPOL` (23), `WIFI_SCAN_ACTIVE_LIST_EAPOL` (28)
- **Entry point:** `WiFiScan::RunEapolScan()` (line 4516)
- **Sniffer callback:** `WiFiScan::eapolSnifferCallback()` (line 9793) — promiscuous mode capture
- **Handshake tracking:** `AccessPoint.has_msg_1` through `has_msg_4` for 4-way handshake completeness
- **PMKID support:** `force_pmkid` setting, `eapol_packet_bad_msg1[153]` template with PMKID IE (OUI 00:0F:AC:04)
- **PCAP output:** `startPcap("eapol")` for capture file writing
- **ESP32-S3 status:** Passive capture via promiscuous mode — fully supported, identical API

### Issue #41 — Evil Portal (Compilation Verified)

Upstream Evil Portal implementation in `EvilPortal.cpp` + `WiFiScan.cpp`:
- **Scan mode:** `WIFI_SCAN_EVIL_PORTAL` (30)
- **Entry point:** `WiFiScan::RunEvilPortal()` (line 3728) → `evil_portal_obj.begin()`
- **Web server:** `ESPAsyncWebServer` + `AsyncTCP` + `DNSServer` (captive portal)
- **Key functions (all linked):**
  - `EvilPortal::setup()` — initialization
  - `EvilPortal::setupServer()` — HTTP route registration
  - `EvilPortal::startAP()` — soft AP creation
  - `EvilPortal::startPortal()` — DNS + web server start
  - `EvilPortal::main()` — DNS request processing loop
  - `EvilPortal::setHtml()` / `setHtmlFromSerial()` — portal page configuration
  - `CaptiveRequestHandler` — catches all HTTP requests for captive portal redirect
- **ESP32-S3 status:** All web server components compile and link — `ESPAsyncWebServer`, `AsyncTCP`, `DNSServer` all present

### `esp_wifi_80211_tx()` Availability on ESP32-S3

Confirmed present in compiled binary at address `0x420c94d4`. Used by:
- Deauth frame injection (`sendDeauthFrame`)
- Beacon frame injection (`RunBeaconScan`)
- Probe request injection
- SAE commit frame injection

API is identical between ESP32 and ESP32-S3 per ESP-IDF documentation.

### Compilation Issues Found

**None.** All four upstream WiFi features compile for ESP32-S3 with zero errors and zero warnings.

---

## Phase 3 Roadmap (Updated)

- [x] Verify promiscuous mode support (Issue #35) — **CONFIRMED**
- [x] Verify WiFiScan.cpp compiles for ESP32-S3 — **CONFIRMED**
- [x] WiFi scan test mode added (ENTER+BACK boot combo)
- [x] Packet monitoring verified for ESP32-S3 (Issue #37) — **CONFIRMED**
- [x] Deauth compilation verified (Issue #38) — **COMPILES**
- [x] Beacon spam compilation verified (Issue #39) — **COMPILES**
- [x] PMKID/EAPOL capture compilation verified (Issue #40) — **COMPILES**
- [x] Evil Portal compilation verified (Issue #41) — **COMPILES**
- [ ] Runtime test: AP scanning on hardware
- [ ] Runtime test: Promiscuous mode packet capture
- [ ] Runtime test: Channel hopping
- [ ] Runtime test: Raw packet injection
- [ ] Runtime test: PCAP file writing to SD
