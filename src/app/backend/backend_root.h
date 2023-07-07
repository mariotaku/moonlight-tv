#pragma once

#include <stdbool.h>

#include "input_manager.h"

typedef struct app_t app_t;

typedef struct app_backend_t {
    app_t *app;
    SDL_mutex *gs_client_mutex;
    input_manager_t input_manager;
} app_backend_t;

void backend_init(app_backend_t *backend, app_t *app);

void backend_destroy(app_backend_t *backend);

bool backend_dispatch_userevent(int which, void *data1, void *data2);