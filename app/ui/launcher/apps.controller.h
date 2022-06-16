#pragma once

#include "lvgl.h"
#include "coverloader.h"
#include "backend/apploader/apploader.h"

typedef struct {
    lv_style_t cover;
    lv_style_t btn;
    lv_style_transition_dsc_t tr_pressed;
    lv_style_transition_dsc_t tr_released;
    lv_img_dsc_t fav_indicator_src;
} appitem_styles_t;

typedef struct {
    lv_fragment_t base;
    apploader_t *apploader;
    coverloader_t *coverloader;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload, *apperror;
    lv_obj_t *errortitle, *errorhint, *errordetail;
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
    lv_sdl_img_data_t cover_data;
    lv_img_dsc_t cover_src;
} appitem_viewholder_t;

extern const lv_fragment_class_t apps_controller_class;