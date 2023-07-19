#pragma once

#include "gamecontrollerdb_updater.h"

typedef struct app_settings_t app_settings_t;

typedef struct input_manager_t {
    commons_gcdb_updater_t gcdb_updater;
} input_manager_t;

void input_manager_init(input_manager_t *manager, const app_settings_t *settings);

