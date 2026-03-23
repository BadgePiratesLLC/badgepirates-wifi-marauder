# Marauder Integration Notes

## Board Config Architecture

The BSidesKC badge uses a layered config approach to stay separate from upstream:

```
include/
  bsideskc_pins.h       ← Physical pin assignments (badge PCB truth)
  bsideskc_config.h     ← Feature flags, board identity
  marauder_config.h     ← Maps badge pins → Marauder names, sets all
                           upstream-expected #defines
```

`marauder_config.h` is the single file that replaces the upstream `configs.h`
board-specific sections. It includes both badge headers and produces every
`#define` that Marauder source files expect (TFT_MOSI, SD_CS, HAS_SCREEN, etc.).

No upstream files are modified. When building, the build system defines
`BSIDESKC_BADGE` and includes `marauder_config.h` instead of activating an
upstream board target in `configs.h`.

## Reference Board: MARAUDER_V8

V8 was chosen because it's the closest match:
- ESP32-S3 with PSRAM
- ILI9341 320×240 TFT with touch
- SD card on separate SPI (HAS_C5_SD)
- GPS, battery, BT/NimBLE

## Deviations from MARAUDER_V8

| Area | V8 | BSidesKC Badge | Reason |
|------|-----|----------------|--------|
| Touch | XPT2046 (SPI resistive) | FT6336U (I2C capacitive) | Badge hardware. TOUCH_CS set to -1; adapter needed |
| Dual-band | HAS_DUAL_BAND | Omitted | Badge ESP32-S3 is single-band 2.4 GHz |
| NeoPixel | Not enabled | HAS_NEOPIXEL_LED (6 LEDs) | Badge has onboard NeoPixels |
| Buttons | Touch-only (no HAS_BUTTONS) | HAS_BUTTONS + rotary encoder | Badge has physical buttons |
| Display SPI pins | V8 uses TFT_eSPI defaults | Explicit TFT_MOSI=11, TFT_SCLK=12, etc. | Badge PCB routing |
| SD SPI bus | Shared via HAS_C5_SD | Dedicated bus (pins 35/36/37/47) | Badge has separate SD SPI |
| GPS pins | TX=14, RX=13 | Same defaults (TBD on badge) | Confirm when GPS header wired |
| Battery I2C | SCL=4, SDA=5 | SCL=9, SDA=8 (shared touch bus) | MAX17048 on same I2C as FT6336U |

## Keeping Configs in Sync with Upstream

1. **Upstream is a git submodule** at `esp32marauder-upstream/`. Update with
   `git submodule update --remote`.

2. **After each upstream update**, diff the V8 sections in `configs.h`:
   ```bash
   git -C esp32marauder-upstream diff HEAD~1 -- esp32_marauder/configs.h | \
     grep -A5 -B5 'MARAUDER_V8'
   ```

3. **Check for new feature flags** — if upstream adds a new `#define` under
   `MARAUDER_V8`, decide whether the badge supports it and add to
   `marauder_config.h`.

4. **Check for new pin expectations** — if upstream references a new pin name
   (e.g., a new peripheral), add the mapping in `marauder_config.h` sourced
   from `bsideskc_pins.h`.

5. **Never edit upstream files directly.** All badge customization lives in
   `include/` and `src/`. This keeps `git submodule update` clean.

## Touch Adapter (Phase 2)

Marauder expects XPT2046-style SPI touch via TFT_eSPI. The badge uses FT6336U
I2C capacitive touch. A shim class will be needed to provide the same
`x, y, pressed` interface. This is tracked separately from the config layer.
