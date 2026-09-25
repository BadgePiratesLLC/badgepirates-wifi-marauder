// LVGL display bring-up, real firmware (Nexus 84f4e52c).
//
// Flushes into display_obj.tft - the SAME TFT_eSPI instance Display.cpp
// already initializes and rotates for CC13's reversed panel (BADGE_HW_CC13,
// Nexus 2977d950). Deliberately NOT LVGL's own lv_tft_espi_create() helper
// (what CC15's QACode_27/src/main.cpp uses): that helper stands up a SECOND,
// independent TFT_eSPI instance with its own default rotation, which would
// disagree with the rotation display_obj.tft is already hardware-validated
// to be running under whenever badgeMenuLoop() owns the screen. One TFT_eSPI
// owner, matching the ticket's own "one buffer, one owner, one push" call
// for sprite compositing - here applied to the display object too.
#include "configs.h"
#ifdef HAS_SCREEN

#include <lvgl.h>
#include "hardware/lv_disp_port.h"
#include "Display.h"
#include "badge_ui_theme.h"

extern Display display_obj;

// alignas(4): LVGL 9.2's lv_display_set_buffers() asserts the buffer is
// LV_DRAW_BUF_ALIGN-aligned (4, see lv_conf.h) - a plain uint8_t[] has no
// alignment guarantee beyond 1 byte, which trips that assert (verified in
// the sim: silent LV_ASSERT_HANDLER while(1) hang with this left off).
alignas(4) static uint8_t s_draw_buf[THEME_LV_DRAW_BUF_BYTES];
static bool s_ready = false;

static uint32_t lv_tick_cb() {
    return millis();
}

// TFT_eSPI's pushColors(data, len, swap) 3rd arg swaps each pixel's byte
// order before clocking it out over SPI - required here because LVGL (with
// this project's LV_COLOR_16_SWAP=0) hands the flush callback native-endian
// RGB565, the same input shape TFT_eSPI's own pushImage()/pushSprite() paths
// expect with swap=true. Unverified on real hardware (Kevin's G6 sign-in
// verification is the next step per this ticket) - if colours come back
// looking channel-swapped/byte-swapped on the actual panel, this is the
// line to flip.
static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    display_obj.tft.startWrite();
    display_obj.tft.setAddrWindow(area->x1, area->y1, w, h);
    display_obj.tft.pushColors((uint16_t*)px_map, w * h, true);
    display_obj.tft.endWrite();
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

#endif
