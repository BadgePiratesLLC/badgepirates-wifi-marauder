# Storage Implementation Notes — Phase 5

## SD Card Hardware Configuration

The BSidesKC badge has an SD card on a **dedicated SPI bus**, separate from the display SPI:

| Signal | Pin | Notes |
|--------|-----|-------|
| MOSI   | 35  | SD_MOSI_PIN |
| SCK    | 36  | SD_SCK_PIN  |
| MISO   | 37  | SD_MISO_PIN |
| CS     | 47  | SD_CS_PIN   |

Display SPI uses pins 11 (MOSI), 12 (SCLK), 10 (CS) — no pin conflicts.

### How the Separate Bus Works

The badge defines `HAS_CYD_TOUCH` and `HAS_SEPARATE_SD` in `marauder_config.h`. In upstream `SDInterface::initSD()`, the `HAS_CYD_TOUCH` preprocessor path:

1. Reads `SD_SCK`, `SD_MISO`, `SD_MOSI` from our pin defines (36, 37, 35)
2. Creates a new `SPIClass()` internally (`spiExt` member)
3. Calls `spiExt->begin(SCK, MISO, MOSI, CS)` then `SD.begin(CS, *spiExt)`

No external SPI object is needed in `main.cpp` — the SDInterface manages it internally.

## Upstream Storage Patterns

### SDInterface (SDInterface.cpp/h)

Core SD abstraction. Key methods:
- `initSD()` — mounts SD, detects card type/size, creates `/SCRIPTS` dir
- `getFile(path)` — opens file for reading
- `removeFile(path)` — deletes a file
- `listDir(path)` — prints directory listing to Serial
- `listDirToLinkedList(list, dir, ext)` — populates LinkedList with filenames, optional extension filter
- `runUpdate(file)` — OTA update from SD card (`/update.bin`)

Global instance: `SDInterface sd_obj;` (no-arg constructor for non-C5 boards).

### Buffer (Buffer.cpp/h) — PCAP Writing

Double-buffered writer for packet captures. Uses PSRAM-allocated buffers (`BUF_SIZE = 8KB`).

**PCAP file format** (standard libpcap):
- Global header: magic `0xa1b2c3d4`, version 2.4, snaplen `SNAP_LEN` (4096), link type 105 (802.11)
- Per-packet: timestamp (sec + usec from `micros()`), included length, original length, payload

**File creation**: Auto-increments filename (`/name_0.pcap`, `/name_1.pcap`, ...).

**Write modes**:
- `pcapOpen()` — PCAP with global header
- `logOpen()` — plain `.log` file (no PCAP header)
- `gpxOpen()` — `.gpx` file (no PCAP header)

**Save targets**: filesystem (`fs::FS*`) and/or serial (`[BUF/BEGIN]...[BUF/CLOSE]` markers).

**Gating**: All writes check `settings_obj.loadSetting<bool>("SavePCAP")`. If false, buffer is disabled.

### WiFiScan PCAP Integration

`WiFiScan.cpp` opens PCAP buffers with:
```cpp
buffer_obj.pcapOpen("filename", sd_obj.supported ? &SD : &SPIFFS, true);
```
Falls back to SPIFFS when SD is unavailable. Serial output always enabled.

## Settings Persistence (settings.cpp/h)

Uses **SPIFFS** (not SD) for settings via `/settings.json`.

**Format**: JSON with ArduinoJson (`DynamicJsonDocument`, size `JSON_SETTING_SIZE = 2048`):
```json
{
  "Settings": [
    {"name": "ForcePMKID", "type": "bool", "value": false, "range": {"min": false, "max": true}},
    {"name": "SavePCAP",   "type": "bool", "value": true,  "range": {"min": false, "max": true}},
    {"name": "EnableLED",  "type": "bool", "value": true,  "range": {"min": false, "max": true}},
    {"name": "EPDeauth",   "type": "bool", "value": false, "range": {"min": false, "max": true}},
    {"name": "ChanHop",    "type": "bool", "value": false, "range": {"min": false, "max": true}},
    {"name": "ClientSSID", "type": "String", "value": "",  "range": {"min": "", "max": ""}},
    {"name": "ClientPW",   "type": "String", "value": "",  "range": {"min": "", "max": ""}}
  ]
}
```

**Auto-create**: Missing settings are appended on first `loadSetting()` call.

**Types supported**: `bool`, `String`, `int`, `uint8_t`.

## Evil Portal HTML Storage

Evil Portal loads HTML templates from **SD card**:
- On construction, scans SD root for `*.html` files via `sd_obj.listDirToLinkedList()`
- `setHtml()` reads `target_html_name` (default: `index.html`) from SD
- Max HTML size: `MAX_HTML_SIZE = 30000` bytes
- HTML stored in PSRAM (`ps_malloc`)
- Fallback: HTML can be set via serial (`sethtml=` command)

AP config file: `/ap.config.txt` on SD for auto-configuring portal SSID.

## SPIFFS Fallback Strategy

The upstream codebase uses a dual-storage approach:
1. **SPIFFS** — always available, used for settings persistence (`/settings.json`)
2. **SD card** — optional, used for PCAP captures, logs, GPX tracks, Evil Portal HTML, OTA updates

When SD is unavailable (`sd_obj.supported == false`):
- PCAP/log/GPX writes fall back to SPIFFS: `sd_obj.supported ? &SD : &SPIFFS`
- Evil Portal HTML must come from serial instead
- OTA updates from SD are unavailable
- Settings continue to work (SPIFFS-only)

## Compilation Status (Issue #46)

**Result: COMPILES SUCCESSFULLY** ✅

```
RAM:   [==        ]  21.0% (used 68928 bytes from 327680 bytes)
Flash: [==        ]  22.4% (used 1469449 bytes from 6553600 bytes)
```

Key findings:
- SD library (`SD @ 2.0.0`) resolves correctly for ESP32-S3
- `SDInterface.cpp` compiles with `HAS_SD`, `HAS_CYD_TOUCH`, `HAS_SEPARATE_SD` defines
- The `spiExt` SPI bus is created internally by `SDInterface::initSD()` — no external `SPIClass` needed in `main.cpp`
- `Buffer.cpp` compiles with PSRAM buffer allocation (`BUF_SIZE = 8KB`, `SNAP_LEN = 4096`)
- `settings.cpp` compiles with SPIFFS and ArduinoJson
- `EvilPortal.cpp` compiles with SD HTML loading path
