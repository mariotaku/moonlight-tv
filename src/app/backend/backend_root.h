#pragma once

#include <stdbool.h>
#include <SDL_thread.h>

typedef struct app_t app_t;
typedef struct executor_t executor_t;

typedef struct app_backend_t {
    app_t *app;
    executor_t *executor;
    SDL_mutex *gs_client_mutex;
} app_backend_t;

void backend_init(app_backend_t *backend, app_t *app);

void backend_destroy(app_backend_t *backend);

bool backend_dispatch_userevent(app_backend_t *backend, int which, void *data1, void *data2);