#include "UI/CardKit.h"

lv_color_t cardkit_color(uint16_t rgb565) {
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    uint8_t r8 = (uint8_t)((r5 * 255 + 15) / 31);
    uint8_t g8 = (uint8_t)((g6 * 255 + 31) / 63);
    uint8_t b8 = (uint8_t)((b5 * 255 + 15) / 31);
    return lv_color_make(r8, g8, b8);
}

static lv_style_t s_style_card;
static lv_style_t s_style_card_pressed;
static bool s_styles_ready = false;

static void ensure_styles() {
    if (s_styles_ready) return;
    s_styles_ready = true;

    lv_style_init(&s_style_card);
    lv_style_set_radius(&s_style_card, THEME_RADIUS_MD);
    lv_style_set_bg_opa(&s_style_card, LV_OPA_COVER);
    lv_style_set_bg_color(&s_style_card, cardkit_color(THEME_SURFACE_GRAD_TOP));
    lv_style_set_bg_grad_color(&s_style_card, cardkit_color(THEME_SURFACE_GRAD_BOTTOM));
    lv_style_set_bg_grad_dir(&s_style_card, LV_GRAD_DIR_VER);
    lv_style_set_border_width(&s_style_card, 1);
    lv_style_set_border_color(&s_style_card, cardkit_color(THEME_BORDER));
    lv_style_set_border_opa(&s_style_card, LV_OPA_COVER);
    lv_style_set_pad_all(&s_style_card, 0);
    lv_style_set_shadow_width(&s_style_card, 0);

    lv_style_init(&s_style_card_pressed);
    lv_style_set_radius(&s_style_card_pressed, THEME_RADIUS_MD);
    lv_style_set_bg_opa(&s_style_card_pressed, LV_OPA_COVER);
    // Flat, near-black fill - deliberately NOT THEME_SURFACE_HI (that
    // token is *lighter* than THEME_SURFACE, a "highlight" tone, the
    // wrong direction for "deepen the fill"; it's still used for the tiny
    // Back control's tap flash, just not here). A flat dark fill with no
    // gradient reads as "pushed in", not a colour swap - "inset the
    // highlight, deepen the fill" from Nexus 84f4e52c.
    lv_style_set_bg_color(&s_style_card_pressed, cardkit_color(THEME_SURFACE_EDGE_LO));
    lv_style_set_bg_grad_dir(&s_style_card_pressed, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&s_style_card_pressed, 1);
    lv_style_set_border_color(&s_style_card_pressed, cardkit_color(THEME_SURFACE_EDGE_LO));
    lv_style_set_border_opa(&s_style_card_pressed, LV_OPA_COVER);
    lv_style_set_pad_all(&s_style_card_pressed, 0);
    lv_style_set_shadow_width(&s_style_card_pressed, 0);
}

static lv_obj_t* create_hairline(lv_obj_t* parent, int16_t x, int16_t y, int16_t w, uint16_t colorToken) {
    lv_obj_t* line = lv_obj_create(parent);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(line, cardkit_color(colorToken), 0);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, w, 1);
    return line;
}

lv_obj_t* cardkit_create_card(lv_obj_t* parent, const CardSpec& spec) {
    ensure_styles();

    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(card, spec.x, spec.y);
    lv_obj_set_size(card, spec.w, spec.h);
    lv_obj_add_style(card, spec.pressed ? &s_style_card_pressed : &s_style_card, 0);

    if (spec.selected) {
        lv_obj_set_style_border_color(card, cardkit_color(THEME_ACCENT), 0);
        lv_obj_set_style_border_width(card, 2, 0);
    }

    // 1px top highlight / bottom shade - "raised surface, not a
    // rectangle". Skipped on the pressed state; its own darker fill+border
    // already reads as inset, a highlight there would fight it.
    if (!spec.pressed) {
        create_hairline(card, THEME_RADIUS_MD, 1, spec.w - THEME_RADIUS_MD * 2, THEME_SURFACE_EDGE_HI);
        create_hairline(card, THEME_RADIUS_MD, spec.h - 2, spec.w - THEME_RADIUS_MD * 2, THEME_SURFACE_EDGE_LO);
    }

    // spec.title == nullptr: caller (e.g. the LED-brightness options grid)
    // wants a custom, centered label instead of this left-aligned one -
    // add nothing here and let the caller build its own child on the
    // returned card object.
    if (spec.title) {
        lv_obj_t* title = lv_label_create(card);
        lv_label_set_text(title, spec.title);
        lv_obj_set_style_text_color(title, cardkit_color(spec.selected ? THEME_ACCENT : THEME_TEXT), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(title, THEME_SPACE_MD, spec.subtitle ? THEME_SPACE_XS : (spec.h - 18) / 2);
    }

    if (spec.subtitle) {
        lv_obj_t* sub = lv_label_create(card);
        lv_label_set_text(sub, spec.subtitle);
        lv_obj_set_style_text_color(sub, cardkit_color(THEME_TEXT_MUTED), 0);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(sub, THEME_SPACE_MD, THEME_SPACE_XS + 18);
    }

    if (spec.navigable) {
        lv_obj_t* chev = lv_label_create(card);
        lv_label_set_text(chev, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(chev, cardkit_color(THEME_TEXT_MUTED), 0);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -THEME_SPACE_SM, 0);
    }

    return card;
}

lv_obj_t* cardkit_create_title(lv_obj_t* parent, const char* title, bool isRoot) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_color(label, cardkit_color(THEME_TEXT), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_update_layout(label);
    lv_obj_set_pos(label, (TFT_WIDTH - lv_obj_get_width(label)) / 2, 4);

    if (isRoot) {
        lv_obj_t* sub = lv_label_create(parent);
        lv_label_set_text(sub, "CC13");
        lv_obj_set_style_text_color(sub, cardkit_color(THEME_TEXT_MUTED), 0);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
        lv_obj_update_layout(sub);
        lv_obj_set_pos(sub, (TFT_WIDTH - lv_obj_get_width(sub)) / 2, THEME_TITLE_H);
    }

    return label;
}

lv_obj_t* cardkit_create_hint(lv_obj_t* parent, const char* text, int16_t centerX, int16_t y) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, cardkit_color(THEME_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_update_layout(label);
    lv_obj_set_pos(label, centerX - lv_obj_get_width(label) / 2, y);
    return label;
}
