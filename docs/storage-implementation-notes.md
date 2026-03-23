# Storage Implementation Notes — Phase 5

Build verified: **SUCCESS** (ESP32-S3, 22.4% Flash, 21.0% RAM)

## Issue #47: PCAP Storage

**Files:** `Buffer.cpp`, `Buffer.h`

The Buffer class implements a dual-buffer PCAP writer with SD card persistence:

- **Double buffering:** Two 8KB buffers (`bufA`/`bufB`) allocated via `malloc()` at construction. When the active buffer nears capacity, writing flips to the other buffer while the full one is flushed to SD.
- **PCAP file creation:** `createFile()` generates unique filenames (`/name_0.pcap`, `/name_1.pcap`, ...) by checking `fs->exists()` to avoid overwrites.
- **PCAP header:** `open(true)` writes the standard libpcap global header (magic `0xa1b2c3d4`, version 2.4, link type 105 = IEEE 802.11).
- **Packet records:** `add()` prepends each packet with timestamp (seconds + microseconds from `micros()`) and length fields per the PCAP record format.
- **Save gating:** `openFile()` checks `settings_obj.loadSetting<bool>("SavePCAP")` — if false, no file is opened and `writing` stays false. The `append()` methods also check this setting before buffering.
- **Filesystem abstraction:** Buffer accepts `fs::FS*`, so it works with SD, SPIFFS, or any FS implementation.
- **Serial output:** `saveSerial()` wraps buffer data in `[BUF/BEGIN]`/`[BUF/CLOSE]` markers for Flipper/serial capture.
- **Badge config:** `BUF_SIZE = 8*1024`, `SNAP_LEN = 4096` (PSRAM-enabled path in `marauder_config.h`).

**Status:** Fully functional. Compiles clean.

## Issue #48: Evil Portal HTML Storage

**Files:** `EvilPortal.cpp`, `EvilPortal.h`

Evil Portal loads and serves HTML files from SD card:

- **HTML discovery:** `setup()` calls `sd_obj.listDirToLinkedList(html_files, "/", "html")` to enumerate all `.html` files on SD root into a `LinkedList<String>`.
- **HTML loading:** `setHtml()` reads the target HTML file (default `index.html`) from SD via `sd_obj.getFile("/" + target_html_name)`. File size is capped at `MAX_HTML_SIZE` (30000 bytes with PSRAM).
- **PSRAM allocation:** With `HAS_PSRAM`, `index_html` is heap-allocated via `ps_malloc(MAX_HTML_SIZE)`. Without PSRAM, it's a static `char[11400]` array.
- **Serial HTML injection:** `setHtmlFromSerial()` allows setting HTML content over serial without SD.
- **Web server:** AsyncWebServer on port 80 serves `index_html` on `/` and all captive portal detection endpoints. Credential capture via `/get` endpoint stores email/password and logs to `buffer_obj.append()`.
- **AP configuration:** Reads AP name from (in priority order): SSID list → selected AP → `/ap.config.txt` on SD.
- **Captive DNS:** DNSServer on port 53 redirects all DNS queries to the portal IP (`172.0.0.1`).

**Status:** Fully functional. Compiles clean.

## Issue #49: Settings Persistence

**Files:** `settings.cpp`, `settings.h`

Settings are persisted to SPIFFS as JSON:

- **Storage backend:** SPIFFS (`/settings.json`). Initialized with `SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)` — auto-formats on first boot.
- **JSON format:** ArduinoJson `DynamicJsonDocument` with `JSON_SETTING_SIZE = 2048`. Each setting has `name`, `type`, `value`, and `range` (min/max) fields.
- **Default settings:** `createDefaultSettings()` writes 8 defaults:
  - `ForcePMKID` (bool, false)
  - `ForceProbe` (bool, false)
  - `SavePCAP` (bool, true) — controls PCAP buffer writing
  - `EnableLED` (bool, true)
  - `EPDeauth` (bool, false)
  - `ChanHop` (bool, false)
  - `ClientSSID` (String, "")
  - `ClientPW` (String, "")
- **Load:** Template-specialized `loadSetting<T>()` for `bool`, `int`, `uint8_t`, `String`. Iterates JSON array to find by name. Auto-creates missing settings.
- **Save:** `saveSetting<bool>()` writes updated JSON back to SPIFFS file and updates in-memory string. `toggleSetting()` flips bool values.
- **In-memory cache:** `json_settings_string` holds the serialized JSON to avoid repeated file reads.

**Status:** Fully functional. Compiles clean.

## Issue #50: SPIFFS Fallback

**Architecture:**

The storage design uses a split-responsibility model rather than a unified fallback:

| Feature | Primary Storage | Fallback |
|---------|----------------|----------|
| Settings | SPIFFS (`/settings.json`) | Auto-create defaults on missing/corrupt |
| PCAP files | SD card (via `Buffer`) | Serial output (`[BUF/BEGIN]`/`[BUF/CLOSE]`) |
| Evil Portal HTML | SD card | Serial injection (`sethtml=`) |
| OTA updates | SD card (`/update.bin`) | None (SD required) |

**SPIFFS partition:** The `default_16MB.csv` partition table includes a 3.375MB SPIFFS partition at offset `0xc90000`. SPIFFS is initialized in `settings_obj.begin()` with `FORMAT_SPIFFS_IF_FAILED = true`.

**SD failure handling:**
- `SDInterface::initSD()` sets `supported = false` on mount failure. All SD operations check this flag.
- `Buffer::openFile()` gracefully handles null `fs` pointer — sets `writing = false`, no crash.
- Evil Portal's `setup()` guards HTML enumeration with `#ifdef HAS_SD` and `sd_obj.supported`.
- Settings are entirely on SPIFFS, independent of SD availability.

**Key insight:** SPIFFS is always available (auto-formatted) for settings. SD is optional for data capture. When SD is absent, PCAP data routes to serial and Evil Portal requires serial HTML injection. This is the upstream Marauder design — no code changes needed.

## Build Verification

```
Platform: espressif32@6.4.0
Board: esp32-s3-devkitc-1
Flash: 22.4% (1,469,481 / 6,553,600 bytes)
RAM:   21.0% (68,960 / 327,680 bytes)
Result: SUCCESS
```

All storage-related libraries resolved:
- `SPIFFS @ 2.0.0`
- `SD @ 2.0.0`
- `FS @ 2.0.0`
- `ArduinoJson @ 7.4.3`
- `ESP Async WebServer @ 2.10.4`
