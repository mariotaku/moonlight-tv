#pragma once

#include "config.h"
#include <SDL_thread.h>
#include "ss4s.h"
#include "client.h"
#include "app_settings.h"
#include "stream/input/session_input.h"
#include "stream/session.h"
#include "embed_wrapper.h"

typedef struct app_t app_t;

struct session_t {
    session_config_t config;

    app_t *app;

    SDL_mutex *state_lock;
    STREAMING_STATE state;
    int display_width, display_height;

    stream_input_t input;
    /* SERVER_DATA and CONFIGURATION is cloned rather than referenced */
    SERVER_DATA *server;
    int app_id;
    char *app_name;
    bool interrupted;
    bool quitapp;
#if FEATURE_EMBEDDED_SHELL
    bool embed;
    embed_process_t *embed_process;
#endif
    SS4S_AudioCapabilities audio_cap;
    SS4S_VideoCapabilities video_cap;
    SDL_cond *cond;
    SDL_mutex *mutex;
    SDL_Thread *thread;
    SS4S_Player *player;
};

void session_set_state(session_t *session, STREAMING_STATE state);

#if FEATURE_EMBEDDED_SHELL
bool session_use_embedded(session_t *session);
#endif