#pragma once

#include "uuidstr.h"

typedef struct app_t app_t;

typedef struct app_launch_params_t {
    uuidstr_t default_host_uuid;
    int default_app_id;
} app_launch_params_t;

app_launch_params_t *app_handle_launch(app_t *app, int argc, char *argv[]);

void app_launch_param_free(app_launch_params_t *param);