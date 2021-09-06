#pragma once

#include <stdbool.h>

#include "ui/config.h"
#include "lvgl/manager.h"

#include "lvgl.h"

#include "backend/pcmanager.h"

typedef struct {
    lv_obj_controller_t base;
    PCMANAGER_CALLBACKS _pcmanager_callbacks;
    lv_obj_t *right;
    lv_obj_t *pclist;
    PSERVER_LIST selected_server;
    uimanager_ctx *pane_manager;
} launcher_controller_t;


lv_obj_t *launcher_win_create(lv_obj_controller_t *self, lv_obj_t *parent);

lv_obj_controller_t *launcher_controller(void *args);