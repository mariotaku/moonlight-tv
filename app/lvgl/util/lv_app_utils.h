#pragma once

#include "lvgl.h"

void lv_obj_set_icon_font(lv_obj_t *obj, const lv_font_t *font);

void lv_btn_set_icon(lv_obj_t *obj, const char *symbol);

lv_obj_t *lv_btn_find_img(lv_obj_t *obj);

lv_obj_t * lv_msgbox_create_i18n(lv_obj_t * parent, const char * title, const char * txt, const char * btn_txts[],
                            bool add_close_btn);