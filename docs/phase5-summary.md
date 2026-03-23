# Phase 5 Summary — Storage & Persistence

## Status: ✅ Complete (compile-verified)

All storage and persistence features compile successfully for the BSidesKC ESP32-S3 badge. Runtime validation requires hardware with SD card.

**Build:** RAM 21.0% (68,928 / 327,680) · Flash 22.4% (1,469,449 / 6,553,600)

---

## Storage Features Verified

| Feature | Source | Status |
|---------|--------|--------|
| SD card SPI interface (#46) | `SDInterface.cpp` | ✅ Compiles |
| PCAP file writing (#47) | `Buffer.cpp` | ✅ Compiles |
| Evil Portal HTML storage (#48) | `EvilPortal.cpp` | ✅ Compiles |
| Settings persistence (#49) | `settings.cpp` | ✅ Compiles |
| SPIFFS fallback (#50) | `WiFiScan.cpp`, `settings.cpp` | ✅ Compiles |

---

## SD Card Configuration

The badge uses a **dedicated SPI bus** for the SD card, separate from the display:

| Signal | GPIO | Define |
|--------|------|--------|
| MOSI | 35 | `SD_MOSI_PIN` |
| SCK | 36 | `SD_SCK_PIN` |
| MISO | 37 | `SD_MISO_PIN` |
| CS | 47 | `SD_CS_PIN` |

Enabled via `HAS_SD`, `HAS_CYD_TOUCH`, and `HAS_SEPARATE_SD` defines in `marauder_config.h`. The `SDInterface::initSD()` method creates its own internal `SPIClass` — no external SPI object needed in `main.cpp`.

---

## PCAP Writing Mechanism

`Buffer.cpp` implements double-buffered PCAP writing:

- **Buffer size:** 8 KB per buffer, PSRAM-allocated (`ps_malloc`)
- **Snap length:** 4,096 bytes
- **Format:** Standard libpcap — magic `0xa1b2c3d4`, version 2.4, link type 105 (802.11)
- **File naming:** Auto-incremented (`/name_0.pcap`, `/name_1.pcap`, ...)
- **Write modes:** `pcapOpen()` (PCAP), `logOpen()` (plain log), `gpxOpen()` (GPX track)
- **Output targets:** Filesystem (`fs::FS*`) and/or serial (`[BUF/BEGIN]...[BUF/CLOSE]` markers)
- **Gating:** All writes check `settings_obj.loadSetting<bool>("SavePCAP")`

---

## Evil Portal HTML Storage

- Scans SD root for `*.html` files via `sd_obj.listDirToLinkedList()`
- Default template: `index.html`
- Max HTML size: 30,000 bytes (PSRAM-allocated)
- AP config: `/ap.config.txt` on SD for portal SSID
- Fallback: HTML can be set via serial (`sethtml=` command) when SD unavailable

---

## Settings Persistence

Uses **SPIFFS** (not SD) via `/settings.json` with ArduinoJson:

```json
{
  "Settings": [
    {"name": "ForcePMKID", "type": "bool", "value": false},
    {"name": "SavePCAP",   "type": "bool", "value": true},
    {"name": "EnableLED",  "type": "bool", "value": true},
    {"name": "EPDeauth",   "type": "bool", "value": false},
    {"name": "ChanHop",    "type": "bool", "value": false},
    {"name": "ClientSSID", "type": "String", "value": ""},
    {"name": "ClientPW",   "type": "String", "value": ""}
  ]
}
```

- JSON document size: 2,048 bytes
- Types: `bool`, `String`, `int`, `uint8_t`
- Auto-creates missing settings on first `loadSetting()` call

---

## SPIFFS Fallback

Dual-storage strategy:

| Storage | Always Available | Used For |
|---------|-----------------|----------|
| SPIFFS | ✅ Yes | Settings (`/settings.json`) |
| SD card | ❌ Optional | PCAP, logs, GPX, Evil Portal HTML, OTA |

When SD unavailable (`sd_obj.supported == false`):
- PCAP/log/GPX writes fall back to SPIFFS: `sd_obj.supported ? &SD : &SPIFFS`
- Evil Portal HTML must come via serial
- OTA from SD unavailable
- Settings unaffected (SPIFFS-only)

---

## What Requires Hardware Testing

These cannot be verified without physical badge hardware:

1. **SD card mount** — SPI bus initialization, card detect, FAT filesystem mount
2. **PCAP file integrity** — write actual captured packets, verify with Wireshark
3. **Evil Portal HTML loading** — read HTML from SD, serve via captive portal
4. **Settings round-trip** — write settings, reboot, verify persistence
5. **SPIFFS capacity** — verify PCAP fallback doesn't exhaust SPIFFS partition
6. **SD + Display SPI coexistence** — both SPI buses operating simultaneously
7. **Write performance** — PCAP buffer flush rate under heavy packet capture

---

## Known Limitations

1. **SPIFFS size constraint** — PCAP fallback to SPIFFS is limited by partition size; large captures will fail
2. **No SD hot-swap** — SD card must be present at boot; no runtime re-detection
3. **Evil Portal requires SD or serial** — no SPIFFS fallback for HTML templates
4. **Single PCAP gating** — `SavePCAP` setting controls all file writes (PCAP, log, GPX)
5. **No SD card format** — no in-firmware SD formatting; card must be pre-formatted FAT32
6. **JSON settings size** — 2 KB limit may constrain future settings additions
