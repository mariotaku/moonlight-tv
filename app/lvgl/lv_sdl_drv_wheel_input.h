#pragma once

#include "lv_conf.h"
#include "lvgl.h"

lv_indev_t *lv_sdl_init_wheel();

void lv_sdl_deinit_wheel(lv_indev_t *dev);