#pragma once

#include "lvgl.h"

typedef lv_obj_t *(*UIMANAGER_WINDOW_CREATOR)(lv_obj_t *parent, const void *args);

void uimanager_pop();

void uimanager_push(lv_obj_t *parent, UIMANAGER_WINDOW_CREATOR creator, const void *args);

void ui_init();

void ui_open_settings();
