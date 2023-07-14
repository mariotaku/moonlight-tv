#pragma once

#include "gamecontrollerdb_updater.h"

typedef struct input_manager_t {
    commons_gcdb_updater_t gcdb_updater;
} input_manager_t;

void inputmgr_init(input_manager_t *manager);

void inputmgr_deinit(input_manager_t *manager);

char *gamecontrollerdb_builtin_path();

char *gamecontrollerdb_extra_path();

char *gamecontrollerdb_fetched_path();

char *gamecontrollerdb_user_path();