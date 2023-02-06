#pragma once

#include <stdbool.h>
#include <SDL_mutex.h>
#include <SDL_thread.h>

typedef struct cec_support_t {
    SDL_mutex *lock;
    SDL_cond *cond;
    SDL_mutex *cond_lock;
    SDL_Thread *thread;
    void *cec_iface;

    bool quit;
} cec_support_ctx_t;

cec_support_ctx_t *cec_support_create();

void cec_support_destroy(cec_support_ctx_t *ctx);