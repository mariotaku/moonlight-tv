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

#if FEATURE_LIBCEC
#include "cec_sdl.h"
#include "ss4s_modules.h"

#endif

extern PCONFIGURATION app_configuration;
extern lv_fragment_manager_t *app_uimanager;
extern pcmanager_t *pcmanager;

typedef struct app_t {
    SDL_Window *window;
    os_info_t os_info;
    app_input_t input;
    struct {
        array_list_t modules;
        module_selection_t selection;
        SS4S_AudioCapabilities audio_cap;
        SS4S_VideoCapabilities video_cap;
    } ss4s;
#if FEATURE_LIBCEC
    cec_sdl_ctx_t cec;
#endif
} app_t;

int app_init(app_t *app, int argc, char *argv[]);

void app_deinit(app_t *app);

void app_init_video();

void app_uninit_video();

void app_handle_launch(int argc, char *argv[]);

void app_process_events();

void app_request_exit();

bool app_is_running();

void app_quit_confirm();

bool ui_render_queue_submit(void *data, unsigned int pts);

GS_CLIENT app_gs_client_new();

void app_set_mouse_grab(bool);

bool app_get_mouse_relative();

void app_set_keep_awake(bool);


void app_set_fullscreen(app_t*app, bool);

void app_open_url(const char *url);

void app_init_locale();

const char *app_get_locale_lang();