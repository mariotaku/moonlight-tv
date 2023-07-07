#pragma once

#include "lv_conf.h"
#include "lvgl.h"

typedef struct app_ui_input_t app_ui_input_t;

int lv_sdl_init_pointer(lv_indev_drv_t *drv, app_ui_input_t *input);

int lv_sdl_init_wheel(lv_indev_drv_t *drv, app_ui_input_t *input);

int lv_sdl_init_button(lv_indev_drv_t *indev_drv, app_ui_input_t *input);

