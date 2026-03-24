# Changelog

## [v1.1.0] — 2025-07-10

### Fixed
- NeoPixel blocking moved to Core 0 task to eliminate display blocking (#61)
- GPS disabled (badge has no GPS module) — saves ~516B RAM + flash

### Removed
- Debug logging stripped for production release

### Added
- SD card troubleshooting guide (#62)
- Production build configuration (`env:bsideskc_prod` in platformio.ini)

### Improved
- ~9.6KB flash savings from debug removal and GPS disable
- F() macro usage for string literals in boot paths

## [v1.0.0] — 2025-03-23

### Initial Release
- ESP32Marauder port for BSidesKC ESP32-S3 badge
- Hardware validated: display, touch, encoder, WiFi, NeoPixels, buzzer, badge menu
- WiFi scanning, deauth, beacon spam, PMKID/EAPOL capture, Evil Portal
- BLE scanning, skimmer detection, BLE spam
- SD card + SPIFFS storage with PCAP saves
- Boot loop fixes and memory optimization for no-PSRAM badge
