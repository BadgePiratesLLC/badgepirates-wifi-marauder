# Deployment Guide — BSidesKC Badge WiFi Marauder

## Prerequisites

### Development Machine
- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) (or PlatformIO IDE)
- Python 3.8+ (for esptool)
- USB-C cable (data-capable, not charge-only)
- Git

### Hardware
- BSidesKC ESP32-S3 badge (BadgePiratesLLC/QACode_27)
- USB-C connection to badge

## Building Firmware

```bash
# Clone repository
git clone https://github.com/BadgePiratesLLC/badgepirates-wifi-marauder.git
cd badgepirates-wifi-marauder

# Build firmware
pio run -e bsideskc-badge

# Output binary: .pio/build/bsideskc-badge/firmware.bin
```

### Build Verification

Successful build output:
```
RAM:   [==        ]  21.1% (used ~69KB from 327KB)
Flash: [==        ]  22.6% (used ~1.48MB from 6.55MB)
========================= [SUCCESS] =========================
```

## Flashing via USB

### First-Time Flash

```bash
# Build and upload in one step
pio run -e bsideskc-badge --target upload

# Monitor serial output to verify boot
pio device monitor -b 115200
```

Expected serial output on successful boot:
```
[BSidesKC] Booting ESP32 Marauder...
[BSidesKC] Rotary encoder initialized
[Buzzer] Initialized on GPIO 19
[Battery] Monitor initialized
[Badge] Menu items added
[Power] Manager initialized
[BSidesKC] Marauder ready.
```

### Manual Flash with esptool

If PlatformIO upload fails, use esptool directly:

```bash
# Install esptool
pip install esptool

# Put badge in download mode: hold BOOT button, press RESET, release BOOT

# Flash firmware
esptool.py --chip esp32s3 --port /dev/cu.usbmodem* \
  --baud 921600 write_flash \
  0x0 .pio/build/bsideskc-badge/bootloader.bin \
  0x8000 .pio/build/bsideskc-badge/partitions.bin \
  0x10000 .pio/build/bsideskc-badge/firmware.bin
```

On macOS, the port is typically `/dev/cu.usbmodem*`. On Linux: `/dev/ttyACM0`.

### OTA Update (Over-the-Air)

After initial USB flash, subsequent updates can be done wirelessly:

1. On badge: navigate to **Device → Update Firmware → Web Update**
2. Badge creates a WiFi AP — connect your laptop/phone to it
3. Open the badge's IP address in a browser
4. Upload the new `firmware.bin` file
5. Badge reboots with updated firmware

## Partition Layout

The badge uses `default_16MB.csv` for the 16MB flash:

| Partition | Type | Offset | Size |
|-----------|------|--------|------|
| nvs | data | 0x9000 | 20 KB |
| otadata | data | 0xE000 | 8 KB |
| app0 | app | 0x10000 | 6.5 MB |
| app1 | app (OTA) | 0x670000 | 6.5 MB |
| spiffs | data | 0xCD0000 | 3 MB |

## SD Card Setup

Format a micro SD card as FAT32 and insert into the badge slot. The firmware auto-creates a `/SCRIPTS` directory on first boot.

Optional files to pre-load:
- `index.html` — Custom Evil Portal page
- `ap.config.txt` — Custom AP SSID for Evil Portal

## Troubleshooting

### Badge won't enter download mode
- Hold BOOT (GPIO 0) before pressing RESET
- Try a different USB-C cable (must support data)
- Check that USB CDC is enabled: `ARDUINO_USB_CDC_ON_BOOT=1` in build flags

### Upload fails with timeout
- Reduce baud rate: `upload_speed = 460800` in `platformio.ini`
- Try `esptool.py` directly with `--baud 115200`

### Display stays black after flash
- Verify TFT_BL pin (GPIO 6) is not held low by another peripheral
- Check serial output for boot messages
- Try holding BOOT during power-on to enter test mode

### Serial monitor shows garbage
- Confirm baud rate is 115200
- Ensure `ARDUINO_USB_CDC_ON_BOOT=1` is set
- On macOS, use the `cu.usbmodem*` port, not `tty.usbmodem*`

### Build fails with missing libraries
```bash
# Clean and rebuild
pio run -e bsideskc-badge --target clean
pio run -e bsideskc-badge
```

## Production Deployment

For flashing multiple badges at a conference:

1. Build firmware once: `pio run -e bsideskc-badge`
2. Copy `.pio/build/bsideskc-badge/firmware.bin` to a shared location
3. Flash each badge:
   ```bash
   esptool.py --chip esp32s3 --port /dev/cu.usbmodem* \
     --baud 921600 write_flash 0x10000 firmware.bin
   ```
4. Verify each badge boots to Marauder menu with serial monitor

### Batch Flash Script

```bash
#!/bin/bash
# flash_badge.sh — Flash a single badge, wait for next
PORT=${1:-/dev/cu.usbmodem*}
FW="firmware.bin"

while true; do
  echo "Connect badge and press Enter (or 'q' to quit)..."
  read -r input
  [ "$input" = "q" ] && break
  esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    write_flash 0x10000 "$FW" && echo "✅ Flash OK" || echo "❌ Flash FAILED"
done
```
