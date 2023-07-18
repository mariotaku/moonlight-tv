#include "backend_root.h"

#include "pcmanager.h"
#include "input_manager.h"
#include "stream/session.h"
#include <SDL2/SDL_cpuinfo.h>
#include <SDL2/SDL_stdinc.h>

pcmanager_t *pcmanager;

void backend_init(app_backend_t *backend, app_t *app) {
    backend->app = app;
    backend->executor = executor_create("moonlight-io", SDL_min(4, SDL_GetCPUCount()));
    backend->gs_client_mutex = SDL_CreateMutex();
    pcmanager = pcmanager_new(app, backend->executor);
    inputmgr_init(&backend->input_manager);
}

void backend_destroy(app_backend_t *backend) {
    inputmgr_deinit(&backend->input_manager);
    pcmanager_destroy(pcmanager);
    SDL_DestroyMutex(backend->gs_client_mutex);
    executor_destroy(backend->executor);
}

bool backend_dispatch_userevent(int which, void *data1, void *data2) {
    bool handled = false;
    return handled;
}