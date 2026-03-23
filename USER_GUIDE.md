# User Guide — BSidesKC Badge WiFi Marauder

## Getting Started

Your BSidesKC badge runs ESP32Marauder, an open-source WiFi and Bluetooth security research tool. This guide covers the badge-specific controls and features.

## Controls

| Control | Location | Action |
|---------|----------|--------|
| Rotary encoder (rotate) | Top of badge | Scroll through menus |
| Rotary encoder (press) | Top of badge | Select menu item |
| ENTER button (GPIO 38) | Right side | Select menu item |
| BACK button (GPIO 39) | Left side | Go back / stop scan |
| BOOT button (GPIO 0) | Bottom | Cycle backlight brightness |
| Touchscreen | Display | Tap to select (in supported screens) |

## Main Menu

After boot, you'll see the Marauder main menu. Use the encoder or touch to navigate:

- **WiFi** — WiFi scanning, attacks, and packet capture
- **Bluetooth** — BLE scanning, skimmer detection, spam
- **Device** — Device info, firmware update, reboot
- **Settings** — Toggle features (SavePCAP, EnableLED, etc.)
- **Badge** — Badge-specific hardware controls
- **Reboot** — Restart the badge

## Badge Menu

The **Badge** submenu provides hardware controls unique to this badge:

### LED Brightness
Adjust the NeoPixel ring brightness. Rotate the encoder to change, press to save. Lower brightness extends battery life.

### Buzzer Mute
Toggle the buzzer on/off. When muted, all audio feedback is silenced.

### Battery Status
Shows current battery percentage with a color-coded bar:
- Green: > 50%
- Yellow: 20–50%
- Red: < 20%

### Hardware Test
Runs the full input validation test (same as holding BOOT during power-on). Tests all buttons, encoder, and touch.

## LED Indicators

The 6 NeoPixels on the badge ring show the current state:

| Pattern | Meaning |
|---------|---------|
| Breathing cyan | Idle — no scan active |
| Rotating blue dot | Scanning (WiFi or BLE) |
| Pulsing red | Attack active (deauth, beacon spam, etc.) |
| Green flash | Successful capture (handshake, PMKID) |
| Red blink | Error condition |

## Buzzer Sounds

| Sound | Meaning |
|-------|---------|
| Short click (2kHz) | Menu selection |
| Rising tone (1k→2k) | Scan/attack started |
| Falling tone (2k→1k) | Scan/attack stopped |
| Triple rising tone | Handshake captured |
| Double low beep (400Hz) | Error |
| Double medium beep (800Hz) | Low battery warning |

## Power Management

The badge automatically manages power to extend battery life:

- **2 minutes idle** → Backlight dims to minimum
- **5 minutes idle** → Badge enters light sleep (display off)
- **Any button press** → Wakes from sleep immediately
- **Active scans** → Prevent auto-sleep while running

To manually cycle backlight brightness, short-press the BOOT button at any time.

## WiFi Features

### Scanning
**WiFi → Scan APs** — Scans for nearby access points. Results show SSID, signal strength, channel, and encryption type.

### Packet Monitor
**WiFi → Packet Monitor** — Real-time display of WiFi traffic on the current channel.

### Deauthentication
**WiFi → Scan APs → Select APs → Deauth** — Sends deauthentication frames to disconnect clients from a selected AP.

### Beacon Spam
**WiFi → Beacon Spam** — Creates fake WiFi networks visible to nearby devices. Options: Random, List, or Target.

### PMKID/Handshake Capture
**WiFi → Scan APs → Select APs → PMKID Scan** — Captures WPA2 handshakes for offline analysis. Saves to SD card as PCAP if available.

### Evil Portal
**WiFi → Evil Portal** — Creates a captive portal AP. Load custom HTML from SD card or via serial.

## Bluetooth Features

### BLE Scan
**Bluetooth → BLE Scan** — Discovers nearby Bluetooth Low Energy devices with addresses, names, and signal strength.

### Skimmer Detection
**Bluetooth → Skimmer Detect** — Scans for BLE devices matching known credit card skimmer signatures.

### BLE Spam
**Bluetooth → BLE Spam** — Sends crafted BLE advertisements. Types: Sour Apple, SwiftPair, Samsung, Google Fast Pair, Flipper.

### AirTag Detection
**Bluetooth → AirTag Scan** — Detects nearby Apple AirTags using continuity protocol parsing.

## SD Card

Insert a FAT32-formatted micro SD card for:
- **PCAP capture** — Packet captures saved as `.pcap` files (open in Wireshark)
- **Evil Portal HTML** — Place `index.html` on SD root for custom portal pages
- **Settings backup** — Settings stored in on-board flash, not SD

Without an SD card, the badge still works — captures fall back to internal flash (limited space).

## Firmware Update

### Over-the-Air (OTA)
1. Navigate to **Device → Update Firmware → Web Update**
2. Connect your phone/laptop to the badge's WiFi AP
3. Open the badge IP in a browser
4. Upload the new firmware `.bin` file
5. Badge reboots automatically

### USB Flash
Connect via USB-C and use PlatformIO:
```bash
pio run -e bsideskc-badge --target upload
```

## Boot Test Modes

Hold buttons during power-on to enter diagnostic modes:

| Hold | Test Mode | Exit |
|------|-----------|------|
| ENTER + BACK | WiFi scan test | Hold BACK |
| BOOT | Full input validation | Hold BOOT + BACK |
| BACK | Encoder test | Hold ENTER |
| ENTER | Touch test | Hold BACK |

## Battery Tips

- Lower LED brightness in Badge → LED Brightness
- Mute buzzer in Badge → Buzzer Mute
- Reduce backlight with BOOT button
- Stop scans when not actively using them
- The badge auto-sleeps after 5 minutes of inactivity

## Serial Console

Connect via USB and open a serial monitor at 115200 baud to see detailed logs:
```bash
pio device monitor -b 115200
```

All scan results, button events, and system messages are logged to serial.

## Responsible Use

This badge is a security research and education tool. WiFi and Bluetooth attack features must only be used on networks and devices you own or have explicit authorization to test. Unauthorized use is illegal. See the [testing guide](docs/testing-guide.md) for detailed legal and ethical guidelines.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Display stays black | Press BOOT to cycle backlight; check if badge is in sleep mode |
| No response to buttons | Hold BOOT + RESET to reboot; try USB reflash |
| WiFi scan finds nothing | Ensure you're in range of WiFi networks; try WiFi scan test mode |
| SD card not detected | Verify FAT32 format; try reinserting card; check serial log |
| Badge won't charge | Try a different USB-C cable; verify cable supports charging |
| Buzzer is silent | Check Badge → Buzzer Mute; buzzer may be muted |
| LEDs not lighting | Check Badge → LED Brightness; may be set to 0% |
