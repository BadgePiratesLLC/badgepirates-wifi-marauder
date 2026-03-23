# Phase 7 Summary — Testing & Polish

Phase 7 completes all development phases for the BSidesKC ESP32-S3 Marauder port with power consumption documentation, UI polish, and final deployment/user documentation.

## Power Consumption Testing (Issue #59)

### Expected Power Consumption by Mode

All estimates based on ESP32-S3 datasheet, ILI9341 display, WS2812B NeoPixels, and MAX17048 fuel gauge. Actual measurements require hardware.

| Mode | Estimated Current | Notes |
|------|------------------|-------|
| Active (screen + WiFi TX) | 180–220 mA | WiFi scanning or attack active |
| Active (screen + BLE scan) | 140–180 mA | BLE scan, no WiFi |
| Idle (screen on, no radio) | 80–120 mA | Menu browsing, radios off |
| Dimmed (min backlight) | 60–90 mA | Backlight at level 1/10 |
| Light sleep | 5–10 mA | Display off, radios off, GPIO wake |
| Deep sleep (not implemented) | ~10 µA | Would require full reboot on wake |

### Component Power Breakdown

| Component | Active | Idle/Off |
|-----------|--------|----------|
| ESP32-S3 CPU (240MHz) | ~60 mA | ~5 mA (light sleep) |
| WiFi TX | ~100–160 mA | 0 mA (off) |
| BLE TX | ~50–80 mA | 0 mA (off) |
| ILI9341 display | ~15–25 mA | ~2 mA (controller idle) |
| TFT backlight (max) | ~30–40 mA | 0 mA (off) |
| TFT backlight (min) | ~3–5 mA | 0 mA (off) |
| NeoPixels 6× (brightness 33) | ~15–25 mA | ~1 mA (idle) |
| Status LED (brightness 50) | ~3–5 mA | ~0.5 mA |
| Buzzer (active tone) | ~10–15 mA | 0 mA |
| MAX17048 fuel gauge | ~0.05 mA | ~0.05 mA |
| FT6336U touch | ~3–5 mA | ~3 mA |
| SD card (active) | ~30–50 mA | ~0.1 mA (idle) |
| Rotary encoder | ~0 mA | ~0 mA (passive) |

### Battery Life Estimates

Assuming a typical 1000 mAh LiPo (badge battery TBD):

| Usage Pattern | Estimated Battery Life |
|---------------|----------------------|
| Continuous WiFi scanning | 4.5–5.5 hours |
| Continuous BLE scanning | 5.5–7 hours |
| Mixed use (scan/browse/idle) | 6–8 hours |
| Mostly idle (menu browsing) | 8–12 hours |
| Idle with auto-dim (2min) | 11–16 hours |
| Light sleep (display off) | 100–200 hours |

### Power Measurement Procedures

1. **Equipment needed:** USB power meter (e.g., Ruideng UM25C) or bench supply with current readout
2. **Baseline:** Measure idle current with badge at main menu, no scans active
3. **WiFi active:** Start WiFi → Scan APs, measure sustained current over 30 seconds
4. **BLE active:** Start Bluetooth → BLE Scan, measure sustained current
5. **Backlight sweep:** Cycle through all 10 brightness levels, record current at each
6. **NeoPixel contribution:** Measure with LEDs on vs. off (Badge menu → LED Brightness → 0%)
7. **Sleep current:** Let badge auto-sleep (5 min idle), measure steady-state current
8. **Wake cycle:** Measure current spike on wake-from-sleep button press

### Power Optimization Recommendations

1. **Default NeoPixel brightness reduced** to 33/255 (~13%) — saves ~40 mA vs full brightness
2. **Auto-dim at 2 minutes** — drops backlight to minimum, saving ~25–35 mA
3. **Auto-sleep at 5 minutes** — enters light sleep at ~5–10 mA
4. **Scan keepalive** — active scans prevent premature sleep
5. **Buzzer PWM** — uses LEDC hardware, no CPU overhead when silent
6. **LED update rate capped at 30fps** — prevents unnecessary NeoPixel SPI traffic
7. **SD card idle** — only accessed during active capture/save operations

**Further optimizations (if battery life insufficient):**
- Reduce auto-dim timeout to 1 minute
- Reduce auto-sleep timeout to 3 minutes
- Add "power saver" mode: disable NeoPixels, reduce scan intervals
- Implement WiFi modem sleep between scan cycles

## UI Polish (Issue #60)

### Display Layout Review (320×240)

All badge-specific screens reviewed and polished for the 240-pixel-wide ILI9341 display:

| Screen | Font | Layout | Status |
|--------|------|--------|--------|
| Boot splash | Font 1 (8px) | Centered, 3 lines at 25/40/55% height | ✅ Fits |
| Battery status | Font 4 (26px) + bar | Centered percentage + progress bar | ✅ Polished |
| LED brightness | Font 2 (16px) + bar | Title + progress bar + percentage | ✅ Polished |
| Buzzer mute toggle | Font 2 (16px) | Centered status message | ✅ Polished |
| Input validation test | Font 1 (8px) | Two-column checklist + live data | ✅ Fits |
| WiFi scan test | Font 1 (8px) | Summary + scrollable AP list | ✅ Fits |
| Encoder test | Font 2 (16px) | Centered position counter | ✅ Fits |
| Touch test | Font 2 + 1 | Title + coordinate readout | ✅ Fits |
| Low battery overlay | Font 2 (16px) | Red bar at top, 16px height | ✅ Fits |

### UI Improvements Made

1. **Standardized UI constants** — `UI_MARGIN`, `UI_TITLE_Y`, `UI_TITLE_FONT`, `UI_BODY_FONT` defined in `display_adapter.h` for consistent layout
2. **Reusable helpers** — `drawCenteredTitle()`, `drawStatusHint()`, `drawProgressBar()` added to `DisplayAdapter` class
3. **Consistent color scheme:**
   - Cyan (`0x07FF`) for titles/headers
   - White (`0xFFFF`) for body text
   - Dark grey (`0x7BEF`) for hints/secondary text
   - Green/Yellow/Red for status indicators
4. **Progress bar component** — Reusable bar with outline, fill, and auto-clear for battery and brightness screens
5. **Fixed duplicate declaration** in `led_feedback.h` (`led_feedback_set_brightness` was declared twice)
6. **Status hints** — All interactive screens show exit instructions at bottom in dim text
7. **Font sizing verified** — Font 2 (16px) for titles fits 15 chars across 240px; Font 1 (8px) for body fits 40 chars

### Menu Layout Verification

Upstream Marauder menu system uses `BUTTON_SCREEN_LIMIT = 12` items per page with `MENU_FONT = FreeMono9pt7b`. On 320×240:
- Menu items: 12 per page × ~22px each = 264px (fits in 320px height with status bar)
- Text width: `STANDARD_FONT_CHAR_LIMIT = 40` chars at 6px = 240px (exact fit)
- Status bar: 16px at top
- Encoder scrolling wraps correctly at page boundaries

## Build Status

```
RAM:   [==        ]  21.1% (used 69264 bytes from 327680 bytes)
Flash: [==        ]  22.6% (used 1483277 bytes from 6553600 bytes)
```

Build size increased by 100 bytes (UI helpers) — negligible impact.

## Files Modified in Phase 7

| File | Change |
|------|--------|
| `src/hardware/display_adapter.h` | Added UI constants and helper method declarations |
| `src/hardware/display_adapter.cpp` | Added `drawCenteredTitle()`, `drawStatusHint()`, `drawProgressBar()` |
| `src/hardware/badge_menu.cpp` | Refactored battery, buzzer, LED screens to use UI helpers |
| `src/hardware/led_feedback.h` | Fixed duplicate `led_feedback_set_brightness` declaration |
| `docs/phase7-summary.md` | New — this document |
| `docs/testing-guide.md` | Updated — Phase 7 power and UI test procedures |
| `DEPLOYMENT.md` | New — deployment instructions |
| `USER_GUIDE.md` | New — end user guide |
| `README.md` | Updated — Phase 7 complete status |
