#pragma once

#include "config.h"
#include <SDL_thread.h>
#include "ss4s.h"
#include "client.h"
#include "stream/settings.h"

#if FEATURE_INPUT_EVMOUSE

#include "evmouse.h"

#endif

#include "stream/input/absinput.h"

typedef struct app_t app_t;

struct session_t {
    app_t *global;
    stream_input_t input;
    /* SERVER_DATA and CONFIGURATION is cloned rather than referenced */
    SERVER_DATA *server;
    CONFIGURATION *config;
    int appId;
    bool interrupted;
    bool quitapp;
    SS4S_VideoCapabilities video_cap;
    SDL_cond *cond;
    SDL_mutex *mutex;
    SDL_Thread *thread;
    SS4S_Player *player;
#if FEATURE_INPUT_EVMOUSE
    struct {
        SDL_Thread *thread;
        evmouse_t *dev;
    } mouse;
#endif
};
