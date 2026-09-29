// LVGL display bring-up, native sim (Nexus 84f4e52c). Same lvDispInit()
// symbol as src/hardware/lv_disp_port.cpp, flushing into the fake
// Framebuffer (gFb) instead of a real TFT_eSPI instance - RGB565 straight
// into Framebuffer::setPixel(), no SPI byte-swap to worry about since
// there's no SPI bus, just host memory.
#include <lvgl.h>
#include "hardware/lv_disp_port.h"
#include "Arduino.h"
#include "Framebuffer.h"
#include "badge_ui_theme.h"

extern Framebuffer gFb;

// alignas(4): see src/hardware/lv_disp_port.cpp's copy of this comment -
// LVGL 9.2 asserts LV_DRAW_BUF_ALIGN alignment on the buffer, a plain
// uint8_t[] doesn't guarantee it.
alignas(4) static uint8_t s_draw_buf[THEME_LV_DRAW_BUF_BYTES];
static bool s_ready = false;

static uint32_t lv_tick_cb() {
    return millis();
}

#ifdef __EMSCRIPTEN__
// Interactive WASM driver only (sim/build_wasm.sh) - see sim_wasm_main.cpp's
// comment on sim_wasm_blit_now() for why this can't just wait for
// frame()'s own end-of-tick blit: a blocking screen's while(true)+delay()
// loop (Settings, Badge submenu, LED Brightness, ...) suspends the call
// stack via Asyncify partway through fixedTick(), so frame() never reaches
// its own blit line until that screen exits. Every flush IS a real visual
// change by definition, so pushing it to the canvas right here is correct
// for every screen, blocking or not, with no per-screen opt-in needed.
extern "C" void sim_wasm_blit_now();
#endif

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    const uint16_t* px = (const uint16_t*)px_map;
    for (int32_t y = area->y1; y <= area->y2; y++) {
        for (int32_t x = area->x1; x <= area->x2; x++) {
            gFb.setPixel(x, y, *px++);
        }
    }
    lv_display_flush_ready(disp);
#ifdef __EMSCRIPTEN__
    sim_wasm_blit_now();
#endif
}

void lvDispInit() {
    if (s_ready) return;
    s_ready = true;

    lv_init();
    lv_tick_set_cb(lv_tick_cb);

    lv_display_t* disp = lv_display_create(THEME_SCREEN_W, THEME_SCREEN_H);
    lv_display_set_buffers(disp, s_draw_buf, nullptr, sizeof(s_draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);
}
