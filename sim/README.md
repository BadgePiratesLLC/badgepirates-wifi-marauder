# Badge UI simulator (Nexus 0f35e128)

Headless, host-native build of the badge's touch-first UI layer — no ESP32,
no cable, no Kevin at a workbench required to see whether a UI change
works. Filed after CC13 touch-UI half 2 (Nexus 78e62be0) shipped a build
that Carla verified 12/12 spec points against, line-by-line, twice — and
was still unusable on the real badge, because nobody had run it. This is
the "run it" step.

## Correcting the ticket's own premise (historical - Nexus 0f35e128, 2026-09-24)

The ticket assumed the badge UI is built on LVGL ("LVGL has a first-class
desktop simulator — compile our own UI layer against it"). It isn't. Grep
the whole firmware tree for `lvgl`/`lv_obj`/`lv_scr` and you get zero hits.
The badge draws directly through **TFT_eSPI** (upstream ESP32Marauder's own
graphics library) — no LVGL anywhere. So instead of an LVGL desktop target,
this is a **native TFT_eSPI-compatible fake**: a host build of the real
`TFT_eSPI`/`TFT_eSPI_Button`/`Display` API surface our code actually calls,
rendering into an in-memory framebuffer instead of an SPI panel. Same goal
(run the UI without hardware), different mechanism, because the premise in
the ticket description doesn't match the codebase.

**No longer true as of Nexus 84f4e52c (2026-09-25): the badge UI IS on LVGL
now.** `src/hardware/badge_menu.cpp`'s card/chrome rendering was ported from
raw TFT_eSPI primitives to LVGL 9.2 widgets (`src/UI/CardKit.cpp`) - real
anti-aliased type, gradient-filled rounded cards, a real pressed state. LVGL
is vendored as a submodule at `sim/vendor/lvgl` (pinned to the same v9.2.2
commit as CC15's QACode_27/sim/vendor/lvgl) and does the actual compositing
now; `sim/fakes/lv_disp_port_sim.cpp` flushes LVGL's draw buffer into the
same `Framebuffer` this file's fakes always wrote into, so everything below
about the framebuffer/PNG capture/touch-scripting machinery is still
accurate. What changed is only WHAT paints into that framebuffer for
badge_menu.cpp's screens - the TFT_eSPI fake immediately below is still
real and still used, just no longer by badge_menu.cpp's own card drawing
(upstream's own screens, reached via the "repro"/"doublefire" modes below,
are unaffected and still TFT_eSPI - out of scope for 84f4e52c, see that
ticket).

## What's real, what's fake

**Compiled unmodified, straight from the firmware tree:**
- `src/hardware/badge_menu.cpp` — the actual card adapter (and the fix from
  this session — see "The bug, reproduced" below)
- `src/hardware/touch_input.cpp` / `include/touch_input.h` — the real
  `TapDetector` debounce logic
- `src/hardware/badge_nav.cpp` / `include/badge_nav.h` — the real back-stack
- `include/badge_ui_theme.h` — the real design tokens
- `include/configs.h` → `marauder_config.h` → `bsideskc_config.h` /
  `bsideskc_pins.h` — pure `#define`s, genuinely portable, no fakes needed

**Fakes, in `sim/fakes/`:**
- `Arduino.h` — millis()/delay()/String/min·max, the subset our files use
- `TFT_eSPI.h` + `Button.h` — draws into `Framebuffer.h`'s RGB888 buffer
  using a hand-authored 5x7 font (`font5x7.h`)
- `Display.h`/`.cpp` — `tft`, `key[]`, `updateTouch()` (fed by the harness's
  scripted taps, not a digitizer), and `menuButton()`
- `MenuFunctions.h`/`.cpp` — see below, this is the one that matters
- `hardware/led_feedback.h` — the real header pulls in `Adafruit_NeoPixel`,
  which doesn't exist on a host; stubbed since the NeoPixel ring isn't part
  of what this ticket needs to prove
- `sim/src/hw_stubs.cpp` — trivial no-op bodies for the real (portable,
  unmodified) `hardware/buzzer.h`, `battery_monitor.h`, `encoder_handler.h`,
  `input_test.h`, `power_manager.h`, `display_adapter.h` declarations

**`MenuFunctions.h`/`.cpp` is a behavioral model, not a port.** The real
`esp32marauder-upstream/esp32_marauder/MenuFunctions.cpp` is 3,828 lines and
drags in WiFiScan/BLE/GPS/SD and dozens of scan-mode branches that have
nothing to do with the UI bug this ticket exists to catch, and are not
host-portable without a much larger stubbing effort than this ticket's
"smallest useful thing first" scope calls for. Instead, `sim/fakes/
MenuFunctions.cpp` reproduces — with the real file and line cited at each
spot — the ONE interaction that actually caused the half-2 hardware
failure: upstream's own touch-read, its own three-invisible-band
UP/SELECT/DOWN gesture model, and its own `changeMenu()`/`displayCurrentMenu()`
full-screen render. Read the comment at the top of that file for the exact
citations. If you're auditing this simulator's fidelity, that file is where
to look — everything else is either real production code or an inert stub.

## What this does NOT cover (read this before trusting a green sim run)

- **Real WiFi/BLE/GPS/SD scan or attack screens.** Only the menu-navigation
  path is modeled. A UI bug inside a scan screen's own rendering will not
  show up here.
- **Real font rendering, panel colour, or viewing angle.** The 5x7 font in
  `font5x7.h` is legible, not a match for TFT_eSPI's actual GLCD/FreeFont
  output on the ILI9341 panel.
- **Real touch calibration.** Touch is a scripted (x, y, down/up) queue, not
  the FT6336U/XPT2046 shim's actual coordinate mapping.
- **Real timing or memory pressure on the ESP32-S3.** The sim runs at
  whatever speed the host CPU gives it; `millis()` only advances when the
  harness calls `sim_advance_millis()`.
- **CH340 flashing, USB enumeration, or the badge dropping off the port.**
  None of that hardware exists here.

A green sim run proves the UI logic does what the code says it does. It
does not replace flashing the real badge — see the ticket's "Done when",
which is still gated on Kevin's photo-and-look approval.

## The bug, reproduced (Nexus 78e62be0 half 2 — cards overlay upstream menu, double/wrong-fire)

Jared's 01:44 diagnosis on half 2's hardware failure named two symptoms:
cards painting over upstream's own menu, and one tap firing more than once
(the buzzer "buzzing"). Reading `MenuFunctions.cpp` directly turned up a
third, worse variant, and this simulator reproduced all of it headless,
before any of the fix below was written:

```
$ ./sim/build.sh && ./sim/out/badge_sim repro sim/out
```

- `repro_00_root_cards.png` — our card-based root, correct.
- `repro_01_upstream_list_after_changeMenu.png` /
  `repro_02_cards_after_badgeMenuLoop.png` — tapping WiFi fires our own
  adapter correctly, whose callable calls upstream's real `changeMenu()`,
  which (`MenuFunctions.cpp:3587-3735`) unconditionally redraws upstream's
  OWN thin-text button list — regardless of who called it. `badgeMenuLoop()`
  doesn't repaint cards until its *next* tick, so for one full `loop()`
  iteration the panel shows upstream's raw list. That's the "cards overlay
  upstream menu" flash, reproduced with no badge attached.
- `doublefire` mode taps "Bluetooth" at a screen position that also falls
  inside one of upstream's own three invisible full-width UP/SELECT/DOWN
  bands (`Display.cpp:14-40`, `MenuFunctions.cpp:464`). Because
  `menu_function_obj.main()` runs every `loop()` tick independently of
  `badgeMenuLoop()`, that tap can fire upstream's OWN touch path directly —
  and it activates `current_menu->selected`, a cursor our touch-first UI
  never moves. Result, confirmed by the sim: **tapping "Bluetooth" silently
  opened "WiFi" instead**, with zero card-side buzz or visual acknowledgment.
  Not just an overlay or a double-fire — a wrong-target navigation, worse
  than either symptom Jared named, and invisible without something watching
  the state (`ourButtonPressBuzzCount` / `changeMenuCallCount` — see
  `sim_counters.h`).

## The fix

Two changes, both in our own files, zero lines touched in
`esp32marauder-upstream`:

1. **`src/hardware/badge_menu.h`/`.cpp`** — extracted `badgeMenuOwnsScreen()`
   (the exact condition `badgeMenuLoop()` already used to no-op itself
   during a scan/attack screen) and exported it.
2. **`src/main.cpp`** — `menu_function_obj.main(currentTime)` now runs only
   when `!badgeMenuOwnsScreen()`. Exactly one of {upstream's own `main()`,
   `badgeMenuLoop()`} touches touch/screen state per tick — never both,
   never a guess about which one "wins" this frame.
3. **`src/hardware/badge_menu.cpp`** — `adaptedActivate()`/`adaptedGoBack()`
   call into upstream's `callable()`/`changeMenu()`, which still draws
   upstream's own screen as a side effect. Waiting for "next tick" to
   repaint cards left that upstream draw as the last thing visibly on the
   panel for a full `loop()` iteration. `adaptedResyncNow()` repaints
   immediately, in the same call that triggered the navigation — upstream's
   draw is never the last frame shown, not "corrected next frame."

Verified in the simulator (not just read):

```
$ ./sim/out/badge_sim fixed sim/out
[fixed] changeMenu() calls for the WiFi tap: 1 (expect 1)
[fixed] menu open after WiFi tap: WiFi (expect WiFi)
[fixed] changeMenu() calls for the Bluetooth tap: 1 (expect 1)
[fixed] menu open after Bluetooth tap: Bluetooth (expect Bluetooth)
[fixed] buzzerPlay(TONE_BUTTON_PRESS) calls for this tap: 1 (expect 1)
```

`fixed_01_after_wifi_tap.png` / `fixed_02_after_bluetooth_tap.png` show our
cards immediately, correctly targeted, no upstream flash — compare against
the `repro_*`/`doublefire_*` PNGs from the unfixed build.

Real firmware still builds clean after the fix:
`pio run -e bsideskc-badge-cc13` → SUCCESS, `firmware.bin` SHA256
`6708288c5802397ac0b6d98d3d101a555cbd7ce117a50880926a25a84b4732dd`.

## Running it

```
./sim/build.sh                        # builds sim/out/badge_sim - needs clang++, zlib, and the sim/vendor/lvgl
                                       # submodule checked out (git submodule update --init sim/vendor/lvgl)
./sim/out/badge_sim root sim/out      # 01_root.png, 02_submenu.png
./sim/out/badge_sim options sim/out   # 03_options.png (LED Brightness grid)
./sim/out/badge_sim pressed sim/out   # 04_pressed.png - a card mid-touch-down (Nexus 84f4e52c: "real pressed state")
./sim/out/badge_sim repro sim/out     # the half-2 bug, reproduced headless (pre-fix code path)
./sim/out/badge_sim doublefire sim/out
./sim/out/badge_sim fixed sim/out     # same scenarios, through the actual fix
```

`root` + `options` together are the ticket's three review screens (root,
a submenu, an options screen); `pressed` is the fourth, added for Nexus
84f4e52c's LVGL port to prove the pressed-state treatment renders and isn't
just a colour swap. `options` calls the real, unmodified
`ledBrightnessOptionsScreen()` directly rather than replaying the full
root → "Badge" card → "LED Brightness" card tap sequence: both of those
navigation steps are blocking input loops (`drawBadgeSubmenu()`, then this
screen) with no tick boundary between them for the harness to script taps
across — `delay()` is a no-op in the sim, so a scripted touch state would
never advance and the loop would spin forever on stale input. Instead,
`badge_menu.cpp` exposes a `SIM_FRAME()` hook (guarded by `SIM_BUILD`,
which only `sim/build.sh` ever defines — absent from the real firmware
build) that fires once, right after the screen's real, unmodified first
render, so `sim_main.cpp` can screenshot it and then arm a fake encoder
press to exit the loop cleanly. Same production drawing code either way;
only the path to reach it differs.

## CI

`.github/workflows/sim-screenshots.yml` builds the simulator and renders
`01_root.png`/`02_submenu.png`/`03_options.png`/`04_pressed.png` as a
downloadable artifact on every push or PR touching the UI layer
(`badge_menu.*`, `touch_input.*`, `badge_nav.*`, `lv_disp_port.*`, `UI/**`,
`badge_ui_theme.h`, `lv_conf.h`, `sim/**`). Checks out submodules
(`sim/vendor/lvgl`, Nexus 84f4e52c) now - it didn't need to before LVGL. It
does **not** auto-attach
to the Nexus ticket — that would need a Nexus API token wired into this
repo's GitHub Actions secrets, which nothing here currently does. Until
that's set up, the loop is: push → open the Actions run → download the
`badge-ui-screenshots` artifact → look. Slower than an inline attachment,
still zero badges and zero workbenches.

## Order of work followed (Jared, Nexus 78e62be0, 01:46)

1. Screenshot mode working end to end — done (`root`, `repro`, `doublefire`,
   `fixed` modes all write PNGs).
2. Reproduce the half-2 bug in it — done, see "The bug, reproduced" above.
   It reproduced on the first faithful model of the real interaction, which
   is itself the strongest evidence the simulator is modeling something
   real and not a strawman.
3. CI PNG generation — done, `.github/workflows/sim-screenshots.yml`, plus
   an `options` mode so all three review screens (root, submenu, options)
   render, not just the two `root` mode already covered. Attachment straight
   onto the Nexus ticket is not done — needs a Nexus API token in this
   repo's Actions secrets that doesn't exist yet; artifact download is the
   loop until that's wired up.
4. Wireframe gate (Kevin approves an SVG mockup before UI code is written)
   and Wokwi CLI evaluation — **not done.** Both are process changes for
   *future* UI work, not something retroactive to apply to the already-
   shipped half-1/half-2 screens this sim renders. Recommend filing as a
   separate follow-up ticket rather than bundling into this one, so it
   doesn't block the fix below from shipping.

## The actual state of Nexus 78e62be0 right now

The bug this sim reproduced and fixed is the exact bug Jared saw on real
hardware when half 2 shipped (cards overlaying upstream's menu, the buzzer
double-firing) — that hardware failure is why this ticket exists. The fix
lives in this repo's `work-sim-0f35e128` branch (`badge_menu.cpp`/`.h`,
`main.cpp`) and, as of this pass, is landing on `main` alongside the
simulator. It is unrelated to Carla's 2026-09-25 03:21–03:26 flash of
`BE1A6D9` — that commit is in the separate `QACode_27` repo (Nexus
7c9c1a15's "desk console" card-schema firmware, no `badge_menu.cpp`, no
shared code with this repo). Landing and reflashing this fix to the actual
CC13/BSidesKC badge is higher priority than the process items above.
