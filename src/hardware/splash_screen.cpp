// Boot splash (Nexus 176cc276). See include/hardware/splash_screen.h.
//
// One opinionated composition per Jared's 07:31 re-scope: logo-dominant,
// skull big enough that the distressed edges actually read, wordmark and
// "MARAUDER BUILD" stacked under it, a version block that's three
// build-time-derived facts (never hand-typed - see the comment on each
// snprintf below), and a discoverable "tap to continue" affordance since
// the hold is meant to be skippable, not just skippable-if-you-know.
//
// Pure LVGL, painted to one screen object and lv_refr_now()'d once - same
// "paint complete, push once" rule UI/CardKit.h documents for every other
// screen in this codebase, applied here to the one screen where it matters
// most: this is what's on the panel before the backlight even comes up.

#include "configs.h"
#ifdef HAS_SCREEN

#include "hardware/splash_screen.h"
#include "hardware/lv_disp_port.h"
#include "badge_ui_theme.h"
#include "UI/CardKit.h"
#include "UI/img_bp_skull.h"
#include "touch_input.h"

// Injected by platformio.ini's scripts/inject_build_info.py (real
// firmware) or sim/build.sh (simulator) - see either for how. Falls back
// to a visibly-wrong placeholder rather than silently omitting the field,
// so a build that skipped the injection is obvious, not just wrong.
#ifndef BP_GIT_SHA
#define BP_GIT_SHA "nosha"
#endif

// 1.2-1.8s target per the ticket; 1500ms sits in the middle. This is a
// ceiling on the REMAINING wait after setup()'s real init work already
// ran - see splashDismissWait() below - not a flat delay stacked on boot.
static const uint32_t SPLASH_MIN_MS = 1500;

lv_obj_t* splashShow() {
  lvDispInit();

  lv_obj_t* screen = lv_obj_create(nullptr);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, cardkit_color(THEME_BG), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(screen, 0, 0);
  lv_obj_set_style_border_width(screen, 0, 0);
  lv_scr_load(screen);

  // The skull: LVGL handles the RGB565A8 alpha compositing itself, so the
  // distressed edge blends against THEME_BG instead of keying onto a hard
  // background - the whole reason this asset carries an alpha plane
  // instead of being flattened at conversion time (scripts/gen_splash_skull.py).
  // That script also inverts the source mark's fill from black to white -
  // the brand asset is a black silhouette built for light/print
  // backgrounds, invisible on this screen's near-black THEME_BG otherwise.
  // Never scaled up from the 270px source; this is scaled DOWN to 132px.
  lv_obj_t* img = lv_image_create(screen);
  lv_image_set_src(img, &img_bp_skull);
  lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 24);

  // Wordmark: the biggest font this no-PSRAM budget buys (montserrat_20,
  // lv_conf.h), tracked out like a title card instead of body copy.
  lv_obj_t* word = lv_label_create(screen);
  lv_label_set_text(word, "BADGE PIRATES");
  lv_obj_set_style_text_color(word, cardkit_color(THEME_TEXT), 0);
  lv_obj_set_style_text_font(word, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_letter_space(word, 2, 0);
  lv_obj_align_to(word, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

  // "Marauder build": Kevin asked for the Marauder build to be named
  // explicitly, so it gets its own accent-coloured line, not folded into
  // the muted version block below.
  lv_obj_t* sub = lv_label_create(screen);
  lv_label_set_text(sub, "MARAUDER BUILD");
  lv_obj_set_style_text_color(sub, cardkit_color(THEME_ACCENT), 0);
  lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_letter_space(sub, 3, 0);
  lv_obj_align_to(sub, word, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  // A single accent-bordered rule - "hacker instrument", not a stock
  // widget-demo splash. Reuses THEME_BORDER, no new colour token.
  lv_obj_t* rule = lv_obj_create(screen);
  lv_obj_remove_flag(rule, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(rule, cardkit_color(THEME_BORDER), 0);
  lv_obj_set_style_border_width(rule, 0, 0);
  lv_obj_set_style_radius(rule, 0, 0);
  lv_obj_set_size(rule, 120, 1);
  lv_obj_align_to(rule, sub, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  // Version block. Every value here is READ, never retyped:
  //  - BP_BUILD_VERSION: our own #define (bsideskc_config.h) - the one
  //    literal in this whole block, bumped by hand when we cut a build.
  //  - MARAUDER_VERSION: upstream's own #define (configs.h chain) - same
  //    define main.cpp's version_number already uses.
  //  - BP_GIT_SHA: injected at build time (see the #ifndef above) - never
  //    a literal anywhere in source.
  // "A boot screen that reports a version the badge is not running is
  // worse than no boot screen" - Nexus 176cc276.
  // Separator is U+2022 BULLET, not U+00B7 MIDDLE DOT - the montserrat
  // font subset this project builds (lv_font_montserrat_12/14/20.c's own
  // header comment: "-r 0x20-0x7F,0xB0,0x2022") carries the bullet, not
  // the middle dot; the wrong one renders as a tofu box (caught rendering
  // this splash in the sim - sim/out/00_splash.png showed it immediately).
  char verLine[48];
  snprintf(verLine, sizeof(verLine), "BP %s" "  \xE2\x80\xA2  " "Marauder %s",
           BP_BUILD_VERSION, MARAUDER_VERSION);
  lv_obj_t* ver = lv_label_create(screen);
  lv_label_set_text(ver, verLine);
  lv_obj_set_style_text_color(ver, cardkit_color(THEME_TEXT_MUTED), 0);
  lv_obj_set_style_text_font(ver, &lv_font_montserrat_12, 0);
  lv_obj_align_to(ver, rule, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

  // Hardware tag + SHA on one line: "which badge, which exact build" is
  // the pair someone holding an unknown badge actually needs (req #5 +
  // the SHA half of the version block).
  char hwLine[40];
  snprintf(hwLine, sizeof(hwLine), "CC13" "  \xE2\x80\xA2  " "%s", BP_GIT_SHA);
  lv_obj_t* hw = lv_label_create(screen);
  lv_label_set_text(hw, hwLine);
  lv_obj_set_style_text_color(hw, cardkit_color(THEME_TEXT_MUTED), 0);
  lv_obj_set_style_text_font(hw, &lv_font_montserrat_12, 0);
  lv_obj_align_to(hw, ver, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

  // Discoverable early-out. The hold itself stays interruptible
  // (splashDismissWait() below) - this just tells the person it's tappable.
  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "tap to continue");
  lv_obj_set_style_text_color(hint, cardkit_color(THEME_TEXT_MUTED), 0);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -THEME_SPACE_MD);

  // Paint the complete frame and push it now - THEME_NO_ARTIFICIAL_DELAY's
  // "instant, no animation budget" rule, applied to the very first thing
  // this badge ever shows. The caller raises the backlight right after
  // this returns; nothing progressive is ever visible.
  lv_refr_now(nullptr);

  return screen;
}

void splashDismissWait(uint32_t startMs, lv_obj_t* screen) {
  while (millis() - startMs < SPLASH_MIN_MS) {
    uint16_t tx, ty;
    if (touchRead(&tx, &ty)) break;
    delay(10);
  }
  if (screen) lv_obj_delete(screen);
}

#endif
