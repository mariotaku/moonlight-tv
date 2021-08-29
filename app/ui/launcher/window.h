#pragma once

#include <stdbool.h>
#include <ui/manager.h>

#include "ui/config.h"

#include "lvgl.h"

#include "util/navkey.h"

#include "backend/computer_manager.h"
#include "backend/application_manager.h"

typedef struct {
    ui_view_controller_t base;
    PCMANAGER_CALLBACKS _pcmanager_callbacks;
    APPMANAGER_CALLBACKS _appmanager_callbacks;
    lv_obj_t *pclist, *applist, *appload;
    PSERVER_LIST selected_server;
} launcher_controller_t;


lv_obj_t *launcher_win_create(launcher_controller_t *controller, lv_obj_t *parent);

ui_view_controller_t *launcher_controller(const void *args);