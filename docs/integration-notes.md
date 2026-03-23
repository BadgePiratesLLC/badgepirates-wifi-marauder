# Badge Menu Integration Design Notes

## Approach
Standalone Marauder firmware first. Dual-boot/menu integration deferred to Phase 6.

## Board Define Strategy
- New `BSIDESKC_BADGE` define in Marauder's `configs.h`
- Closest upstream target: `MARAUDER_V8` (ESP32-S3 + ILI9341 + touch + PSRAM)
- Fork pin mappings from V8, override with badge-specific pins

## Touch Adapter
- FT6336U (I2C capacitive) replaces XPT2046 (SPI resistive)
- Adapter must provide `x, y, pressed` interface matching Marauder expectations
- Library candidate: `Adafruit_FT6206` or direct I2C register reads

## Display
- Same ILI9341 controller as upstream — TFT_eSPI User_Setup.h pin changes only
- 320×240 landscape orientation

## SD Card
- Badge has dedicated SD SPI bus (separate from display SPI)
- Full PCAP capture capability available

## NeoPixel Feedback (Phase 6)
- Scan active: breathing blue
- Attack running: pulsing red
- Idle: low white
- Battery low: amber flash
