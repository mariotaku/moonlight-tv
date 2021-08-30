#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/config.h"
#include "ui/manager.h"
#include "util/navkey.h"

#include "lvgl.h"

#include "stream/settings.h"
#include "app.h"

typedef struct {
    ui_view_controller_t base;
    uimanager_ctx *pane_manager;
    lv_obj_t *nav, *detail;
} settings_controller_t;

lv_obj_t *settings_win_create(struct ui_view_controller_t *self, lv_obj_t *parent);

ui_view_controller_t *settings_controller(const void *args);