#pragma once

#include "lvgl.h"
#include "coverloader.h"
#include "backend/apploader/apploader.h"
#include "util/uuidstr.h"

typedef struct {
    lv_style_t cover;
    lv_style_t btn;
    lv_style_transition_dsc_t tr_pressed;
    lv_style_transition_dsc_t tr_released;
    lv_img_dsc_t fav_indicator_src;
    lv_img_dsc_t defcover_src;
} appitem_styles_t;


typedef struct {
    lv_fragment_t base;
    uuidstr_t uuid;
    const pclist_t *node;

    int def_app;
    bool def_app_launched;

    apploader_t *apploader;
    coverloader_t *coverloader;
    apploader_cb_t apploader_cb;

    apploader_list_t *apploader_apps;
    const char *apploader_error;

    lv_obj_t *applist, *appload, *apperror;
    lv_obj_t *errortitle, *errorhint, *errordetail;
    lv_obj_t *actions;

    lv_obj_t *quit_progress;

    appitem_styles_t appitem_style;
    int col_count;
    lv_coord_t col_width, col_height;
    int focus_backup;
} apps_fragment_t;

typedef struct {
    apploader_item_t *app;
    apps_fragment_t *controller;
    const appitem_styles_t *styles;
    lv_obj_t *play_indicator;
    lv_obj_t *title;
} appitem_viewholder_t;

typedef struct {
    uuidstr_t host;
    int def_app;
} apps_fragment_arg_t;

extern const lv_fragment_class_t apps_controller_class;