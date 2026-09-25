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

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    const uint16_t* px = (const uint16_t*)px_map;
    for (int32_t y = area->y1; y <= area->y2; y++) {
        for (int32_t x = area->x1; x <= area->x2; x++) {
            gFb.setPixel(x, y, *px++);
        }
    }
    lv_display_flush_ready(disp);
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
