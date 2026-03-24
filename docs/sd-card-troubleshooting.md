# SD Card Troubleshooting Guide

## Known Issue

On boot, the badge logs:
```
Card Failed! cmd: 0x00
f_mount failed: (3)
SD Card NOT Supported
```

The badge continues to operate normally — SPIFFS provides fallback storage for settings and PCAP captures.

## Pin Configuration

SD card uses a dedicated SPI bus (separate from the display):

| Signal | GPIO | Define     |
|--------|------|------------|
| MOSI   | 35   | `SD_MOSI`  |
| SCK    | 36   | `SD_SCK`   |
| MISO   | 37   | `SD_MISO`  |
| CS     | 47   | `SD_CS`    |

Display SPI uses GPIO 10–12, so there is no bus conflict.

## How SD Init Works

`HAS_CYD_TOUCH` and `HAS_SEPARATE_SD` are defined, so `SDInterface::initSD()`:
1. Creates a new `SPIClass()` instance
2. Calls `spiExt->begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS)` — (36, 37, 35, 47)
3. Calls `SD.begin(SD_CS, *spiExt)`

## Error Meaning

- `cmd: 0x00` — SPI command GO_IDLE_STATE failed; card did not respond to the initial reset
- `f_mount failed: (3)` — FatFS `FR_NOT_READY`, the physical drive cannot work (no card or no response)

## Possible Causes

1. **No SD card inserted** — expected behavior, not a bug
2. **Wrong pin mapping** — GPIO 35–37 on ESP32-S3 may be strapping/input-only pins depending on the module variant; verify against the actual PCB schematic
3. **Card format** — must be FAT32; exFAT and NTFS are not supported
4. **Card incompatibility** — some high-capacity (SDXC) or UHS-III cards fail SPI mode init
5. **SPI clock too fast** — the default `SPIClass()` constructor uses HSPI at 4 MHz, which should be fine, but a damaged card may need slower speeds
6. **Hardware wiring** — cold solder joint or missing pull-up on MISO/CS

## Testing Steps

1. **Confirm no card inserted** — if the slot is empty, the error is expected
2. **Try a known-good card** — use a ≤32 GB microSD formatted FAT32 (allocation unit 32K)
3. **Check serial output** — look for `"Using external SPI configuration..."` before the failure
4. **Verify GPIO with logic analyzer** — probe CS(47) for a low pulse and SCK(36) for clock activity during `SD.begin()`
5. **Test with reduced SPI speed** — in `SDInterface.cpp`, after `spiExt->begin(...)`, add:
   ```cpp
   spiExt->setFrequency(1000000); // 1 MHz
   ```
6. **Check pin strapping** — on ESP32-S3, GPIO 45/46 are strapping pins; GPIO 35–37 should be safe but confirm with the module datasheet

## Workaround

SPIFFS fallback is fully functional:
- Settings persist to `/settings.json` on SPIFFS
- PCAP/log writes use `sd_obj.supported ? &SD : &SPIFFS`
- All Marauder features work without an SD card

## Related Issues

- GitHub Issue #22: Configure SD card SPI interface
- GitHub Issue #26: Configure SPIFFS as fallback storage
