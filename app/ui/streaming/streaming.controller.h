#pragma once

#include "lvgl/manager.h"

typedef struct {
    lv_obj_controller_t base;
    lv_obj_t *progress;
    lv_obj_t *suspend_btn;
    lv_obj_t *quit_btn;
} streaming_controller_t;

lv_obj_controller_t *streaming_controller(void *args);

lv_obj_t *streaming_scene_create(lv_obj_controller_t *self, lv_obj_t *parent);