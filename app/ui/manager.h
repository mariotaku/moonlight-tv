#pragma once

#include "lvgl.h"

typedef lv_obj_t *(*UIMANAGER_WINDOW_CREATOR)();

void uimanager_pop();

void ui_init();

void ui_open_settings();