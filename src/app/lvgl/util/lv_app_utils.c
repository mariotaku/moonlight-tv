//
// Created by Mariotaku on 2021/09/15.
//

#include "lv_app_utils.h"

#include "util/i18n.h"

static void msgbox_i18n_destroy(lv_event_t *e);


lv_obj_t *lv_child_find_type(lv_obj_t *obj, const lv_obj_class_t *cls);

void lv_btn_set_text_font(lv_obj_t *obj, const lv_font_t *font) {
    lv_obj_t *child = lv_btn_find_label(obj);
    if (!child) return;
    lv_obj_set_style_text_font(child, font, 0);
}

void lv_btn_set_icon_font(lv_obj_t *obj, const lv_font_t *font) {
    lv_obj_t *child = lv_btn_find_img(obj);
    if (!child) return;
    lv_obj_set_style_text_font(child, font, 0);
}

void lv_btn_set_icon(lv_obj_t *obj, const char *symbol) {
    lv_obj_t *child = lv_btn_find_img(obj);
    if (!child) return;
    lv_img_set_src(child, symbol);
}

lv_obj_t *lv_btn_find_img(lv_obj_t *obj) {
    if (!lv_obj_has_class(obj, &lv_btn_class)) return NULL;
    return lv_child_find_type(obj, &lv_img_class);
}

lv_obj_t *lv_btn_find_label(lv_obj_t *obj) {
    if (!lv_obj_has_class(obj, &lv_btn_class)) return NULL;
    return lv_child_find_type(obj, &lv_label_class);
}

lv_obj_t *lv_child_find_type(lv_obj_t *obj, const lv_obj_class_t *cls) {
    for (int i = 0, j = (int) lv_obj_get_child_cnt(obj); i < j; ++i) {
        struct _lv_obj_t *child = lv_obj_get_child(obj, i);
        if (lv_obj_has_class(child, cls)) {
            return child;
        }
    }
    return NULL;
}


lv_obj_t *lv_msgbox_create_i18n(lv_obj_t *parent, const char *title, const char *txt, const char *btn_txts[],
                                bool add_close_btn) {
    int btn_txts_len;
    for (btn_txts_len = 0; btn_txts[btn_txts_len][0]; btn_txts_len++);
    const char **btn_txts_loc = lv_mem_alloc(sizeof(char *) * (btn_txts_len + 1));
    for (int i = 0; i < btn_txts_len; i++) {
        btn_txts_loc[i] = locstr(btn_txts[i]);
    }
    btn_txts_loc[btn_txts_len] = "";
    lv_obj_t *msgbox = lv_msgbox_create(parent, title, txt, btn_txts_loc, add_close_btn);
    lv_obj_add_event_cb(msgbox, msgbox_i18n_destroy, LV_EVENT_DELETE, btn_txts_loc);
    return msgbox;
}

static void msgbox_i18n_destroy(lv_event_t *e) {
    lv_mem_free(lv_event_get_user_data(e));
}