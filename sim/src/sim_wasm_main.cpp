// BSidesKC Badge simulator — interactive WASM driver (Nexus 7b1f65b2).
//
// Same real UI code sim_main.cpp's headless screenshot modes run
// (badgeMenuSetup()/badgeMenuLoop(), badge_menu.cpp/touch_input.cpp/
// badge_nav.cpp, unmodified), same fakes (sim/fakes/), same LVGL
// compositing into gFb (sim/fakes/lv_disp_port_sim.cpp) — just driven by
// requestAnimationFrame instead of a scripted tap list, and fed by real
// mouse clicks on an HTML canvas instead of sim_main.cpp's hardcoded
// coordinates. This file is a driver on top of the existing harness, not
// a new harness: it never touches badge_menu.cpp, touch_input.cpp,
// badge_nav.cpp, or sim/fakes/ — only sim/build_wasm.sh links this instead
// of sim_main.cpp+png_writer.h.
//
// The gating mirrors sim_main.cpp's fixedTick() exactly (main.cpp's real
// post-78e62be0-fix loop() shape): exactly one of {upstream's own
// menu_function_obj.main(), badgeMenuLoop()} touches touch/screen state
// per tick, decided by badgeMenuOwnsScreen() — this is the sim mode Kevin
// clicking around should see, not the deliberately-pre-fix repro/doublefire
// unconditional-tick modes.

#include <cstdint>
#include <cstdio>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "configs.h"
#include "Arduino.h"
#include "Display.h"
#include "MenuFunctions.h"
#include "hardware/badge_menu.h"
#include "hardware/splash_screen.h"
#include "sim_counters.h"
#include "Framebuffer.h"
#include "badge_ui_theme.h"
#include "UI/CardKit.h"

Framebuffer gFb;
SimState g_sim;
Print Serial;

static uint32_t s_millis = 0;
uint32_t millis() { return s_millis; }
void sim_advance_millis(uint32_t ms) { s_millis += ms; }

// badge_menu.cpp's SIM_FRAME() hook target (see sim_main.cpp's own copy of
// this comment) — headless screenshot mode uses it to capture
// ledBrightnessOptionsScreen()'s blocking input loop from the inside.
// Interactive mode never needs that: the browser's own requestAnimationFrame
// loop already renders every frame of every screen, blocking loops included
// (frame() below re-enters through fixedTick() -> badgeMenuLoop() and
// friends each tick, same as the real firmware's loop()), so this is a
// deliberate no-op, not an unimplemented seam.
void sim_capture_frame(const char*) {}

// Blit gFb (RGB888, row-major) onto the page's <canvas id="badge">. Reads
// straight out of wasm linear memory (HEAPU8) — no intermediate copy on
// the JS side beyond what ImageData already requires.
EM_JS(void, js_blit_frame, (const uint8_t* rgb, int w, int h), {
  if (!Module.__badgeCanvas) {
    Module.__badgeCanvas = document.getElementById('badge');
    Module.__badgeCtx = Module.__badgeCanvas.getContext('2d');
    Module.__badgeImgData = Module.__badgeCtx.createImageData(w, h);
  }
  const src = HEAPU8.subarray(rgb, rgb + w * h * 3);
  const dst = Module.__badgeImgData.data;
  for (let i = 0, j = 0; i < w * h; i++, j += 3) {
    const o = i * 4;
    dst[o] = src[j]; dst[o + 1] = src[j + 1]; dst[o + 2] = src[j + 2]; dst[o + 3] = 255;
  }
  Module.__badgeCtx.putImageData(Module.__badgeImgData, 0, 0);
});

static double s_lastFrameMs = -1;

static void fixedTick() {
  if (!badgeMenuOwnsScreen()) menu_function_obj.main(millis());
  badgeMenuLoop();
}

static void frame() {
  double now = emscripten_get_now();
  if (s_lastFrameMs < 0) s_lastFrameMs = now;
  double deltaMs = now - s_lastFrameMs;
  s_lastFrameMs = now;
  if (deltaMs < 0) deltaMs = 0;
  if (deltaMs > 250) deltaMs = 250;  // clamp a tab-backgrounded stall so millis() doesn't jump absurdly
  sim_advance_millis((uint32_t)deltaMs);

  fixedTick();

  js_blit_frame(gFb.rgb.data(), gFb.w, gFb.h);
}

extern "C" {

// Called from JS on canvas mousedown/mousemove/mouseup — canvas-space
// pixel coordinates already, see sim/wasm/index.html's scaling math. Feeds
// the exact same Display::updateTouch() seam sim_main.cpp's scripted
// sim_set_touch() calls feed (sim/fakes/Display.cpp), so real mouse input
// exercises the same TapDetector debounce/hit-test code a finger on the
// real FT6336U does.
EMSCRIPTEN_KEEPALIVE
void sim_wasm_pointer(int down, int x, int y) {
  sim_set_touch(down != 0, (uint16_t)x, (uint16_t)y);
}

EMSCRIPTEN_KEEPALIVE
int sim_wasm_screen_w() { return THEME_SCREEN_W; }

EMSCRIPTEN_KEEPALIVE
int sim_wasm_screen_h() { return THEME_SCREEN_H; }

}  // extern "C"

int main() {
  gFb.init(THEME_SCREEN_W, THEME_SCREEN_H);
  menu_function_obj.RunSetup();
  badgeMenuSetup();  // real production code, unmodified

  // fps=0 + simulate_infinite_loop=1: let the browser drive via
  // requestAnimationFrame instead of a fixed rate, and never return to
  // main()'s caller — em_main_loop keeps main()'s locals (none here) and
  // the module alive for the life of the page.
  emscripten_set_main_loop(frame, 0, 1);
  return 0;
}
