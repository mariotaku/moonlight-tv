#include "app.h"
#include "util/bus.h"

#include <SDL.h>
#include <assert.h>

typedef struct bus_blocking_action_t {
    bus_actionfunc action;
    void *data;
    SDL_mutex *mutex;
    SDL_cond *cond;
    bool done;
} bus_action_sync_t;

static void invoke_action_sync(bus_action_sync_t *sync);

bool bus_pushevent(int which, void *data1, void *data2) {
    SDL_Event ev;
    ev.type = SDL_USEREVENT;
    ev.user.code = which;
    ev.user.data1 = data1;
    ev.user.data2 = data2;
    SDL_PushEvent(&ev);
    return true;
}

bool app_bus_post(app_t *app, bus_actionfunc action, void *data) {
    assert(action != NULL);
    if (!app->running) {
        return false;
    }
    return bus_pushevent(BUS_INT_EVENT_ACTION, action, data);
}

bool app_bus_post_sync(app_t *app, bus_actionfunc action, void *data) {
    bus_action_sync_t sync = {
            .action = action,
            .data = data,
            .mutex = SDL_CreateMutex(),
            .cond = SDL_CreateCond(),
            .done = false,
    };
    if (!app_bus_post(app, (bus_actionfunc) invoke_action_sync, &sync)) {
        SDL_DestroyMutex(sync.mutex);
        SDL_DestroyCond(sync.cond);
        return false;
    }
    SDL_LockMutex(sync.mutex);
    while (!sync.done) {
        SDL_CondWait(sync.cond, sync.mutex);
    }
    SDL_UnlockMutex(sync.mutex);
    SDL_DestroyMutex(sync.mutex);
    SDL_DestroyCond(sync.cond);
    return true;
}

void app_bus_drain() {
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_USEREVENT, SDL_USEREVENT) > 0) {
        if (event.user.code != BUS_INT_EVENT_ACTION) {
            continue;
        }
        bus_actionfunc actionfn = event.user.data1;
        actionfn(event.user.data2);
    }
}

static void invoke_action_sync(bus_action_sync_t *sync) {
    SDL_LockMutex(sync->mutex);
    sync->action(sync->data);
    sync->done = true;
    SDL_CondSignal(sync->cond);
    SDL_UnlockMutex(sync->mutex);
}