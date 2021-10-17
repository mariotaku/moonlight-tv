#pragma once

#include <backend/apploader/apploader.h>
#include "lvgl/ext/lv_obj_controller.h"
#include "coverloader.h"

typedef struct {
    lv_style_t cover;
    lv_style_t btn;
    lv_style_transition_dsc_t tr_pressed;
    lv_style_transition_dsc_t tr_released;
    char fav_indicator_src[LV_SDL_IMG_LEN];
} appitem_styles_t;

typedef struct {
    lv_obj_controller_t base;
    apploader_t *apploader;
    coverloader_t *coverloader;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload, *apperror;
    lv_obj_t *errortitle, *errorlabel;
    lv_obj_t *actions;

    appitem_styles_t appitem_style;
    int col_count;
    lv_coord_t col_width, col_height;
    int focus_backup;
} apps_controller_t;

typedef struct {
    apploader_item_t *app;
    apps_controller_t *controller;
    lv_obj_t *play_indicator;
    lv_obj_t *title;
    char cover_src[LV_SDL_IMG_LEN];
} appitem_viewholder_t;

extern const lv_obj_controller_class_t apps_controller_class;