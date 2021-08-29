#pragma once

#include "ui/manager.h"

typedef struct {
    ui_view_controller_t base;
    lv_obj_t *progress;
} streaming_controller_t;

ui_view_controller_t *streaming_controller(const void *args);

lv_obj_t *streaming_scene_create(streaming_controller_t *controller, lv_obj_t *parent);