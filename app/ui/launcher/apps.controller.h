#pragma once

#include <backend/apploader/apploader.h>
#include "lvgl/ext/lv_obj_controller.h"
#include "coverloader.h"
#include "appitem.view.h"

typedef struct {
    lv_obj_controller_t base;
    apploader_t *apploader;
    coverloader_t *coverloader;
    PSERVER_LIST node;
    lv_obj_t *applist, *appload, *apperror;
    lv_obj_t *errortitle, *errorlabel;
    lv_obj_t *wol_btn;

    lv_obj_t *retry_btn;
    appitem_styles_t appitem_style;
    int col_count;
    lv_coord_t col_width, col_height;
    int focus_backup;
} apps_controller_t;

extern const lv_obj_controller_class_t apps_controller_class;