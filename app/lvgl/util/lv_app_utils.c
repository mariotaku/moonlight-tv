//
// Created by Mariotaku on 2021/09/15.
//

#include "lv_app_utils.h"

void lv_obj_set_icon_font(lv_obj_t *obj, const lv_font_t *font) {
    uint32_t child_cnt = lv_obj_get_child_cnt(obj);
    for (int i = 0; i < child_cnt; ++i) {
        struct _lv_obj_t *child = lv_obj_get_child(obj, i);
        if (lv_obj_has_class(child, &lv_img_class)) {
            lv_obj_set_style_text_font(child, font, 0);
        }
    }
}

void lv_btn_set_icon(lv_obj_t *obj, const char *symbol) {
    if (!lv_obj_has_class(obj, &lv_btn_class))return;
    uint32_t child_cnt = lv_obj_get_child_cnt(obj);
    for (int i = 0; i < child_cnt; ++i) {
        struct _lv_obj_t *child = lv_obj_get_child(obj, i);
        if (lv_obj_has_class(child, &lv_img_class)) {
            lv_img_set_src(child, symbol);
        }
    }
}