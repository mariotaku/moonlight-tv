#pragma once

#include <SDL.h>

#include "config.h"

#include "lvgl.h"
#include "stream/settings.h"
#include "backend/pcmanager.h"
#include "libgamestream/client.h"
#include "os_info.h"
#include "array_list.h"
#include "ss4s_modules.h"
#include "ss4s.h"
#include "input/app_input.h"
#include "backend/backend_root.h"
#include "ui/root.h"

#if FEATURE_INPUT_LIBCEC

#include "cec_sdl.h"
#include "ss4s_modules.h"

#endif

extern app_t *global;

extern PCONFIGURATION app_configuration;
extern pcmanager_t *pcmanager;

typedef struct session_t session_t;

typedef struct app_t {
    SDL_threadID main_thread_id;
    os_info_t os_info;
    app_backend_t backend;
    app_input_t input;
    app_ui_t ui;
    struct {
        array_list_t modules;
        SS4S_ModuleSelection selection;
        SS4S_AudioCapabilities audio_cap;
        SS4S_VideoCapabilities video_cap;
    } ss4s;
#if FEATURE_INPUT_LIBCEC
    cec_sdl_ctx_t cec;
#endif
    session_t *session;
} app_t;

int app_init(app_t *app, int argc, char *argv[]);

void app_deinit(app_t *app);

int app_loop(app_t *app);

void app_init_video();

void app_uninit_video();

void app_handle_launch(int argc, char *argv[]);

void app_process_events(app_t *app);

void app_request_exit();

bool app_is_running();

void app_quit_confirm();

bool ui_render_queue_submit(void *data, unsigned int pts);

GS_CLIENT app_gs_client_new(app_t *app);

void app_set_mouse_grab(bool);

bool app_get_mouse_relative();

void app_set_keep_awake(bool);

void app_set_fullscreen(app_t *app, bool);

void app_open_url(const char *url);

void app_init_locale();

const char *app_get_locale_lang();
