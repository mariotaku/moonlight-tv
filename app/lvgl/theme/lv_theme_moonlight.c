#include "lv_theme_moonlight.h"

static void apply_cb(struct _lv_theme_t *, lv_obj_t *);

void lv_theme_moonlight_init(lv_theme_t *theme) {
    lv_theme_set_apply_cb(theme, apply_cb);
    theme->font_small = &lv_font_montserrat_28;
    theme->font_large = &lv_font_montserrat_38;
}

static void apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
    if (lv_obj_has_class(obj, &lv_btn_class)) {
        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, 0);
    }
    if (lv_obj_has_class(obj, &lv_list_btn_class)) {
        lv_obj_set_style_bg_opa(obj, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_STATE_FOCUS_KEY);
//        lv_obj_set_style_border_width(obj, lv_dpx(2), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_color(obj, theme->color_primary, LV_STATE_FOCUS_KEY);
    }
    if (lv_obj_has_class(obj, &lv_label_class)) {
        lv_obj_t *parent = lv_obj_get_parent(obj);
        if (parent) {
            lv_obj_t *parent2 = lv_obj_get_parent(parent);
            if (lv_obj_has_class(parent2, &lv_win_class) && lv_win_get_header(parent2) == parent) {
                lv_obj_set_style_text_font(obj, theme->font_large, 0);
            }
        }
    }
}