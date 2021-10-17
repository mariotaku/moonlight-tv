#pragma once

#include <backend/apploader/apploader.h>
#include "lvgl.h"
#include "client.h"

#include "lvgl/lv_sdl_img.h"

typedef struct {
    lv_style_t cover;
    lv_style_t btn;
    lv_style_transition_dsc_t tr_pressed;
    lv_style_transition_dsc_t tr_released;
} appitem_styles_t;

typedef struct {
    apploader_item_t *app;
    lv_obj_t *play_indicator;
    lv_obj_t *title;
    char cover_src[LV_SDL_IMG_LEN];
} appitem_viewholder_t;


lv_obj_t *appitem_view(lv_obj_t *parent, appitem_styles_t *styles);

void appitem_style_init(appitem_styles_t *style);