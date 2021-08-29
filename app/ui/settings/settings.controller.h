#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/config.h"
#include "ui/manager.h"
#include "util/navkey.h"

#include "lvgl.h"

#include "stream/settings.h"
#include "app.h"

lv_obj_t *settings_win_create(struct ui_view_controller_t *controller, lv_obj_t *parent);

ui_view_controller_t *settings_controller(const void *args);