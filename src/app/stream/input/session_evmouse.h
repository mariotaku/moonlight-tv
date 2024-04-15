#pragma once

#include <SDL_thread.h>

typedef struct session_t session_t;

typedef struct session_evmouse_t {
    session_t *session;
    SDL_mutex *lock;
    SDL_cond *cond;
    SDL_Thread *thread;
    struct evmouse_t *dev;
} session_evmouse_t;

void session_evmouse_init(session_evmouse_t *mouse, session_t *session);

void session_evmouse_deinit(session_evmouse_t *mouse);

void session_evmouse_wait_ready(session_evmouse_t *mouse);

void session_evmouse_interrupt(session_evmouse_t *mouse);