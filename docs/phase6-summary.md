# Phase 6 Summary — Badge Integration

Phase 6 implements all badge-specific hardware integration features for the BSidesKC ESP32-S3 badge, completing the bridge between upstream ESP32Marauder functionality and the badge's unique hardware.

## Features Implemented

### NeoPixel LED Feedback (Issue #51)

**Files:** `src/hardware/led_feedback.h`, `src/hardware/led_feedback.cpp`

6× WS2812B NeoPixels (GPIO 18) and 1× status LED (GPIO 21) provide visual state feedback synchronized with Marauder scan modes.

| LED State | Pattern | Trigger |
|-----------|---------|---------|
| `LED_IDLE` | Breathing cyan | Menu idle, no active scan |
| `LED_SCANNING` | Rotating blue dot | WiFi/BLE scan active |
| `LED_ATTACK` | Pulsing red | Deauth, beacon spam, BLE spam |
| `LED_CAPTURE_OK` | Green flash (600ms) | PMKID/EAPOL captured |
| `LED_ERROR` | Red blink (600ms) | Error condition |

- Brightness set to 33/255 (ring) and 50/255 (status) for battery conservation
- Update rate capped at ~30fps to minimize CPU overhead
- Transient states (CAPTURE_OK, ERROR) auto-return to previous state

### Buzzer Audio Feedback (Issue #52)

**Files:** `src/hardware/buzzer.h`, `src/hardware/buzzer.cpp`

PWM tone output on GPIO 19 via LEDC channel 2 (channel 0 reserved for backlight).

| Tone | Pattern | Use |
|------|---------|-----|
| `TONE_BUTTON_PRESS` | 2kHz 30ms | Menu selection |
| `TONE_SCAN_START` | 1kHz→2kHz rising | Scan/attack started |
| `TONE_SCAN_STOP` | 2kHz→1kHz falling | Scan/attack stopped |
| `TONE_CAPTURE_SUCCESS` | 1.5k→2.5k→3.5k triple | Handshake captured |
| `TONE_ERROR` | 400Hz double beep | Error condition |
| `TONE_LOW_BATTERY` | 800Hz double beep | Battery ≤15% |

- Async pattern engine — `buzzerUpdate()` in main loop drives multi-note sequences
- Global mute via `buzzerMute(true)` — silences all tones
- Patterns defined as freq/duration pairs, zero-copy

### Battery Monitoring (Issue #53)

**Files:** `src/hardware/battery_monitor.h`, `src/hardware/battery_monitor.cpp`

Wraps upstream `BatteryInterface` (MAX17048 fuel gauge on shared I2C bus) with low-battery warnings.

- **Low battery** (≤15%): Single warning tone, serial log
- **Critical battery** (≤5%): Repeated warning every 30s, red status bar overlay
- `batteryGetPercent()` / `batteryGetVoltage()` for programmatic access
- I2C shared with FT6336U touch controller (SDA:8, SCL:9)

### Menu Integration (Issue #54)

Upstream Marauder menu system (`MenuFunctions`) fully operational with badge inputs:

- Rotary encoder (A:45, B:48) drives menu scrolling with page computation
- Encoder button (GPIO 20) and ENTER button (GPIO 38) both select items
- BACK button (GPIO 39) navigates back
- BOOT button (GPIO 0) cycles backlight brightness
- Buzzer feedback on every menu selection
- All upstream menu paths accessible: WiFi, Bluetooth, Device, Settings

### OTA Update Support (Issue #55)

Upstream Marauder OTA is available via the existing menu path:

**Menu:** Device → Update Firmware → Web Update

- Partition table (`default_16MB.csv`) includes OTA partitions
- Marauder's built-in `OTA_UPDATE` / `ESP_UPDATE` scan modes handle the update flow
- Badge creates WiFi AP, serves web UI for firmware upload
- Progress displayed on screen via upstream Display methods
- LED state set to `LED_IDLE` during OTA (no interference)

**Note:** OTA runtime behavior requires hardware testing to confirm the web update server binds correctly on ESP32-S3.

### Power Management (Issue #56)

**Files:** `src/hardware/power_manager.h`, `src/hardware/power_manager.cpp`

Battery-conscious power management with auto-sleep.

| Feature | Behavior |
|---------|----------|
| Backlight dim | After 2 minutes of inactivity, backlight drops to minimum |
| Auto-sleep | After 5 minutes of inactivity, enters ESP32-S3 light sleep |
| Wake-on-button | Any button press (BOOT, ENTER, BACK, encoder) wakes from sleep |
| Activity reset | Any user input or active scan resets the inactivity timer |
| Scan keepalive | Active WiFi/BLE scans prevent auto-sleep |

**Power consumption expectations (estimated, requires hardware measurement):**

| Mode | Estimated Current |
|------|------------------|
| Active (screen + WiFi scan) | ~180-220mA |
| Idle (screen on, no radio) | ~80-120mA |
| Dimmed (min backlight) | ~60-90mA |
| Light sleep | ~5-10mA |
| NeoPixels (6× at brightness 33) | ~15-25mA additional |

## Build Status

All Phase 6 code compiles cleanly with `pio run -e bsideskc-badge`.

## What Requires Hardware Testing

All Phase 6 features are compile-verified but require physical badge hardware for runtime validation:

1. **NeoPixel patterns** — Verify colors, brightness, animation smoothness on actual WS2812B LEDs
2. **Buzzer tones** — Verify audible output, volume levels, frequency accuracy on badge piezo
3. **Battery readings** — Verify MAX17048 I2C communication, percentage accuracy, voltage readings
4. **I2C bus sharing** — Confirm touch + battery gauge coexist without bus contention
5. **OTA update** — Confirm web update server starts, firmware upload succeeds, rollback works
6. **Light sleep** — Verify wake-on-button works reliably, peripheral re-init after wake
7. **Power consumption** — Measure actual current draw in each mode with multimeter
8. **LED + buzzer during scans** — Verify no timing interference with WiFi/BLE operations

## Files Added/Modified in Phase 6

| File | Change |
|------|--------|
| `src/hardware/led_feedback.h` | New — LED state enum and API |
| `src/hardware/led_feedback.cpp` | New — NeoPixel patterns and state machine |
| `src/hardware/buzzer.h` | New — Buzzer tone enum and API |
| `src/hardware/buzzer.cpp` | New — PWM tone patterns with async engine |
| `src/hardware/battery_monitor.h` | New — Battery monitoring API |
| `src/hardware/battery_monitor.cpp` | New — Low/critical battery warnings |
| `src/hardware/power_manager.h` | New — Power management API |
| `src/hardware/power_manager.cpp` | New — Auto-sleep, dimming, wake-on-button |
| `src/main.cpp` | Modified — Integrated all Phase 6 subsystems |
| `include/bsideskc_config.h` | Existing — Feature flags (HAS_NEOPIXEL_LED, HAS_BATTERY) |
| `include/bsideskc_pins.h` | Existing — Pin definitions for all badge hardware |
