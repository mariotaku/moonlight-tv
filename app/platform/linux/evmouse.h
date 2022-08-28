#pragma once

#include <SDL.h>

typedef struct evmouse_t evmouse_t;

typedef union evmouse_event_t {
    Uint32 type;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
} evmouse_event_t;

typedef void (*evmouse_listener_t)(const evmouse_event_t *event, void*userdata);

evmouse_t *evmouse_open_default();

void evmouse_close(evmouse_t *mouse);

void evmouse_listen(evmouse_t *mouse,evmouse_listener_t listener, void* userdata);

void evmouse_interrupt(evmouse_t *mouse);