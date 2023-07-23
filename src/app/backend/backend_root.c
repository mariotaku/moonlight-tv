#include "backend_root.h"

#include "pcmanager.h"
#include "stream/session.h"
#include <SDL2/SDL_cpuinfo.h>
#include <SDL2/SDL_stdinc.h>

#include "app.h"
#include "executor.h"

pcmanager_t *pcmanager;


void backend_init(app_backend_t *backend, app_t *app) {
    backend->app = app;
    backend->executor = executor_create("moonlight-io", 2 * SDL_min(3, SDL_GetCPUCount()));
    backend->gs_client_mutex = SDL_CreateMutex();
    pcmanager = pcmanager_new(app, backend->executor);
}

void backend_destroy(app_backend_t *backend) {
    pcmanager_destroy(pcmanager);
    SDL_DestroyMutex(backend->gs_client_mutex);
    executor_destroy(backend->executor);
}

bool backend_dispatch_userevent(app_backend_t *backend, int which, void *data1, void *data2) {
    (void) backend;
    (void) which;
    (void) data1;
    (void) data2;
    return false;
}