#pragma once

#include "lv_conf.h"
#include "lvgl.h"

lv_indev_t *lv_sdl_init_pointer_input(void);
void lv_sdl_deinit_pointer_input(void);