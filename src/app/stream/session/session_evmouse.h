#pragma once

#include <SDL_thread.h>

typedef struct session_t session_t;

typedef struct session_evmouse_t {
    SDL_Thread *thread;
    struct evmouse_t *dev;
} session_evmouse_t;

void session_evmouse_init(session_t *session);

void session_evmouse_deinit(session_t *session);

void session_evmouse_interrupt(session_t *session);