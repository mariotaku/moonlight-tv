#include "lunasynccall.h"

#include <libhelpers.h>
#include <SDL.h>

struct HContextSync {
    HContext base;
    SDL_mutex *mutex;
    SDL_cond *cond;
    bool finished;
    char **output;
};

static bool callback(LSHandle *sh, LSMessage *reply, void *ctx);

bool HLunaServiceCallSync(const char *uri, const char *payload, bool public, char **output) {
    struct HContextSync context = {
            .base.multiple = 0,
            .base.public = public ? 1 : 0,
            .base.callback = callback,
            .mutex = SDL_CreateMutex(),
            .cond = SDL_CreateCond(),
            .output = output,
    };
    if (HLunaServiceCall(uri, payload, &context.base) != 0) {
        SDL_DestroyMutex(context.mutex);
        SDL_DestroyCond(context.cond);
        return false;
    }
    SDL_LockMutex(context.mutex);
    while (!context.finished) {
        SDL_CondWait(context.cond, context.mutex);
    }
    SDL_UnlockMutex(context.mutex);

    SDL_DestroyMutex(context.mutex);
    SDL_DestroyCond(context.cond);
    return true;
}

static bool callback(LSHandle *sh, LSMessage *reply, void *ctx) {
    struct HContextSync *context = (struct HContextSync *) ctx;
    context->finished = true;
    if (context->output) {
        *context->output = strdup(HLunaServiceMessage(reply));
    }
    SDL_CondSignal(context->cond);
    return true;
}