#pragma once

#include "lv_conf.h"
#include "lvgl.h"

lv_indev_t *lv_sdl_init_key_input();

void lv_sdl_key_input_release_key(lv_indev_t *indev);

void lv_sdl_deinit_key_input(lv_indev_t *indev);

lv_indev_t *lv_sdl_init_pointer();

void lv_sdl_deinit_pointer(lv_indev_t *dev);

lv_indev_t *lv_sdl_init_wheel();

void lv_sdl_deinit_wheel(lv_indev_t *dev);

lv_indev_t *lv_sdl_init_button();

void lv_sdl_deinit_button(lv_indev_t *dev);