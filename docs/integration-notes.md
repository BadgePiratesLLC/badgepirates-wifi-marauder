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
| Touch | XPT2046 (SPI resistive) | FT6336U (I2C capacitive) | Badge hardware. Shimmed via `HAS_CYD_TOUCH` path |
| Dual-band | HAS_DUAL_BAND | Omitted | Badge ESP32-S3 is single-band 2.4 GHz |
| NeoPixel | Not enabled | HAS_NEOPIXEL_LED (6 LEDs) | Badge has onboard NeoPixels |
| Buttons | Touch-only (no HAS_BUTTONS) | HAS_BUTTONS + rotary encoder | Badge has physical buttons |
| Display SPI pins | V8 uses TFT_eSPI defaults | Explicit TFT_MOSI=11, TFT_SCLK=12, etc. | Badge PCB routing |
| SD SPI bus | HAS_C5_SD (shared SPI) | HAS_SEPARATE_SD + HAS_CYD_TOUCH path | CYD SD path avoids C5_SD/CYD_TOUCH conflict in SDInterface |
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

## Touch Adapter (Phase 2) — Implemented

### Problem

Marauder expects XPT2046 SPI resistive touch. The badge uses FT6336U I2C
capacitive touch (SDA:8, SCL:9, RST:3, INT:7). These are completely different
hardware interfaces.

### Solution: XPT2046 Shim via `HAS_CYD_TOUCH`

Rather than modifying upstream Display.cpp, we exploit the existing
`HAS_CYD_TOUCH` code path. Upstream's CYD touch path uses an
`XPT2046_Touchscreen` object with methods `tirqTouched()`, `touched()`,
`getPoint()`, `begin()`, and `setRotation()`.

We provide a drop-in replacement:

```
include/XPT2046_Touchscreen.h   ← Shadows the real XPT2046 library header.
                                    Defines TS_Point struct and
                                    XPT2046_Touchscreen class backed by FT6336U.
src/hardware/touch_adapter.h     ← FT6336U init + global accessor
src/hardware/touch_adapter.cpp   ← FT6336U instance (pins from bsideskc_pins.h)
```

### How It Works

1. `marauder_config.h` defines `HAS_CYD_TOUCH` + dummy XPT2046 pin values.
2. Upstream `Display.h` does `#include <XPT2046_Touchscreen.h>` — our
   `include/` directory is first in the search path, so our shim is used.
3. Our shim class wraps FT6336U reads and scales raw 0-239/0-319 coordinates
   to the 200-3700/240-3800 ADC range that Display.cpp's `map()` calls expect.
4. Upstream `Display::updateTouch()` works unmodified — it calls our shim's
   `tirqTouched()`, `touched()`, `getPoint()` and gets correct screen coords.

### Coordinate Mapping

FT6336U returns portrait coordinates (0-239 X, 0-319 Y). The shim scales
these to match the resistive ADC range. Display.cpp's rotation-aware `map()`
then converts to screen coordinates for the current TFT rotation.

### SD Card Interaction

Defining `HAS_CYD_TOUCH` conflicts with `HAS_C5_SD` in upstream SDInterface.h
(they use `#elif`). The badge now uses `HAS_SEPARATE_SD` instead, which is the
CYD-compatible path for a dedicated SD SPI bus. The SD init in SDInterface.cpp
uses the `HAS_CYD_TOUCH || HAS_SEPARATE_SD` branch with `SD_SCK`/`SD_MISO`/
`SD_MOSI`/`SD_CS` pins from `marauder_config.h`.

### Touch Test Mode

Hold the ENTER button (GPIO 38) during boot to enter touch test mode. This
draws cyan dots where you touch and shows coordinates in the top-right corner.
Hold BACK (GPIO 39) to exit and continue normal boot.

### Reference

QACode_27's `Screen_Module.cpp` was used as reference for FT6336U I2C init
and coordinate reading patterns on the same badge hardware.
