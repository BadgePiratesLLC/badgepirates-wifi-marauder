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

## Rotary Encoder (Phase 2, Issue #33) — Implemented

### Hardware

The badge has an EC11-style rotary encoder with push button:
- Pin A (CLK): GPIO 45
- Pin B (DT):  GPIO 48
- Button (SW): GPIO 20

Pins defined in `bsideskc_pins.h` as `ENC_A_PIN`, `ENC_B_PIN`, `ENC_BTN_PIN`.

### Library

Uses `ESP32RotaryEncoder` by MaffooClock (v1.1.0), already in `platformio.ini`
lib_deps. The library handles interrupt-driven encoder reading with debouncing
via an internal ESP timer.

### Implementation

```
src/hardware/encoder_handler.h   ← Public API: init, turned_up, turned_down, button_pressed
src/hardware/encoder_handler.cpp ← ESP32RotaryEncoder wrapper with volatile flags
```

Pattern follows QACode_27's `RotaryEncoder_Module.cpp`:
- Encoder bounded to -1/1, value reset to 0 after each turn event
- ISR callbacks set volatile flags for direction and button press
- Polling functions consume flags (read-and-clear)

### Menu Integration

Encoder is integrated in `main.cpp` loop, **after** `menu_function_obj.main()`
runs (so physical buttons and touch still work normally).

The encoder navigation uses only public MenuFunctions methods:
- `current_menu->selected` — directly updated for selection index
- `buildButtons(menu, start_index)` — redraws button list from given index
- `displayCurrentMenu(start_index)` — renders the visible page

This avoids modifying upstream code. The private `buttonSelected()` /
`buttonNotSelected()` methods are not accessible, so encoder turns trigger a
full page rebuild via `buildButtons` + `displayCurrentMenu`.

Rotation mapping:
- CW (clockwise)  → scroll down (increment selected, wrap to 0)
- CCW (counter-CW) → scroll up (decrement selected, wrap to end)
- Button press     → execute selected menu item's callable

Navigation only activates when Marauder is in menu mode (`WIFI_SCAN_OFF`,
`WIFI_CONNECTED`, or `OTA_UPDATE`). During active scans, encoder events are
ignored (physical ENTER/BACK buttons handle scan stop).

### Encoder Test Mode

Hold the BACK button (GPIO 39) during boot to enter encoder test mode.
The screen shows a position counter that increments/decrements with rotation.
Button presses are logged to Serial. Hold ENTER (GPIO 38) to exit and
continue normal boot.

### Reference

QACode_27's `RotaryEncoder_Module.cpp` was the reference implementation for
the same badge hardware encoder using the same ESP32RotaryEncoder library.

## Badge Menu Integration (Phase 6, Issue #54) — Implemented

### Problem

Upstream Marauder's menu system has no awareness of badge-specific hardware
(NeoPixel ring brightness, buzzer mute, battery gauge, hardware test modes).
The `MenuFunctions` class keeps `deviceMenu` and `addNodes()` private, so we
cannot inject items from outside the class.

### Solution: Post-RunSetup Menu Injection

After `menu_function_obj.RunSetup()` completes, `current_menu` (public) points
to `mainMenu`. We directly manipulate the LinkedList to insert a "Badge"
top-level menu item before the existing "Reboot" entry.

```
src/hardware/badge_menu.h    ← Public API: badgeMenuSetup()
src/hardware/badge_menu.cpp  ← Badge submenu with 4 items + main menu injection
```

### Menu Structure

```
Main Menu
  ├── WiFi          (upstream)
  ├── Bluetooth     (upstream)
  ├── GPS           (upstream, if detected)
  ├── Device        (upstream: firmware update, brightness, info, settings)
  ├── Badge         ← NEW
  │   ├── Back
  │   ├── LED Brightness    — Rotary encoder adjusts NeoPixel ring + status LED
  │   ├── Buzzer Mute       — Toggle buzzer on/off
  │   ├── Battery Status    — Full-screen percentage + bar graph
  │   └── Hardware Test     — Runs input validation test (touch, buttons, encoder)
  └── Reboot        (upstream)
```

### How It Works

1. `badgeMenuSetup()` is called in `setup()` immediately after
   `menu_function_obj.RunSetup()`.
2. A static `badgeMenu` (Menu struct) is created with its own LinkedList of
   MenuNode entries.
3. The "Reboot" node is temporarily removed from mainMenu's list, "Badge" is
   appended, then "Reboot" is re-added — preserving it as the last item.
4. Each badge menu item uses `menu_function_obj.changeMenu()` (public) to
   navigate back to the badge submenu after completing its action.

### Badge Menu Items

- **LED Brightness**: Interactive screen with a fill bar. Rotary encoder
  adjusts brightness through 8 levels (5→255). Uses `led_feedback_set_brightness()`
  to update both the 6× NeoPixel ring and status LED in real-time.
- **Buzzer Mute**: Toggles `buzzerMute()`. Shows confirmation text for 800ms.
- **Battery Status**: Reads `batteryGetPercent()` from the MAX17048 fuel gauge.
  Displays percentage in large text with a color-coded bar (green/yellow/red).
  Press encoder button or BACK to return.
- **Hardware Test**: Launches `runInputValidationTest()` — the same test
  available by holding BOOT during startup.

### Rotary Encoder Navigation

The existing encoder integration in `main.cpp` loop handles navigation of the
badge submenu identically to all other menus — no special handling needed.
The encoder code operates on `menu_function_obj.current_menu` which
automatically points to `badgeMenu` when the user enters it.

### No Upstream Modifications

All badge menu code lives in `src/hardware/badge_menu.cpp`. The only change
to `main.cpp` is adding the `#include` and calling `badgeMenuSetup()` after
`RunSetup()`.

## OTA Updates (Phase 6, Issue #55) — Verified

### Upstream OTA Implementation

Marauder's OTA-from-SD is implemented in `SDInterface::runUpdate()` and
`SDInterface::performUpdate()` in the upstream submodule. The implementation:

1. Opens `/update.bin` (or a user-selected file) from the SD card
2. Uses Arduino's `Update` library (`Update.begin()`, `Update.writeStream()`,
   `Update.end()`)
3. Uses `esp_ota_ops.h` to set the next boot partition
   (`esp_ota_get_next_update_partition()`, `esp_ota_set_boot_partition()`)
4. Reboots via `ESP.restart()`

### ESP32-S3 Compatibility

The OTA implementation is fully compatible with ESP32-S3:
- `<Update.h>` is part of the ESP32 Arduino core and supports all ESP32
  variants including S3
- `esp_ota_ops.h` is from ESP-IDF, which is the foundation for ESP32-S3
- The partition table (`default_16MB.csv`) includes OTA partitions
- No S3-specific code paths are needed — the upstream code works as-is

### Menu Access

OTA update is accessible via: **Main Menu → Device → Update Firmware**.
This is the upstream menu item that calls `sd_obj.runUpdate()` after
presenting an SD file browser filtered for `.bin` files.

### OTA Update Procedure

1. Build firmware: `pio run` produces `.pio/build/bsideskc-badge/firmware.bin`
2. Copy `firmware.bin` to SD card root as `/update.bin`
3. Insert SD card into badge
4. Navigate to **Device → Update Firmware** and select the file
5. Badge displays progress, writes to the next OTA partition, and reboots
6. On successful boot, the new partition becomes active

### Limitations

- **No web OTA**: The upstream web update (`ESP_UPDATE` mode) requires
  connecting to a WiFi network and hosting a web server. This works but is
  less practical for badge use — SD update is preferred.
- **No rollback UI**: If an OTA update fails to boot, the ESP32-S3 bootloader
  will fall back to the previous partition automatically (ESP-IDF default
  behavior with `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`).
- **File size**: The 16MB flash with default partition table supports firmware
  up to ~6.5MB per OTA slot. Current firmware is ~1.5MB.

### Serial OTA Alternative

For development, `pio run --target upload` flashes directly over USB-CDC.
This bypasses the SD card entirely and is faster for iterative development.
