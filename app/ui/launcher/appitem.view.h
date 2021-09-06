#pragma once

#include "lvgl.h"
#include "backend/appmanager.h"

typedef struct {
    lv_style_t cover;
    lv_style_t btn;
    lv_style_transition_dsc_t tr_pressed;
    lv_style_transition_dsc_t tr_released;
} appitem_styles_t;

typedef struct {
    PAPP_DLIST app;
    lv_obj_t *play_btn;
    lv_obj_t *close_btn;
    char cover_src[64];
} appitem_viewholder_t;


lv_obj_t *appitem_view(lv_obj_t *parent, appitem_styles_t *styles);

void appitem_style_init(appitem_styles_t *style);