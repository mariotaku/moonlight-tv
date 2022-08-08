#pragma once

#include <SDL.h>

#include "lvgl.h"
#include "module/api.h"
#include "stream/settings.h"
#include "backend/pcmanager.h"
#include "libgamestream/client.h"

extern PCONFIGURATION app_configuration;
extern lv_fragment_manager_t *app_uimanager;
extern pcmanager_t *pcmanager;
extern SDL_Window *app_window;

int app_init(int argc, char *argv[]);

void app_init_video();

void app_uninit_video();

void app_process_events();

void app_request_exit();

bool app_is_running();

void app_quit_confirm();

void app_start_text_input(int x, int y, int w, int h);

void app_stop_text_input();

bool app_text_input_active();

bool ui_render_queue_submit(void *data, unsigned int pts);

GS_CLIENT app_gs_client_new();

void app_set_mouse_grab(bool);

bool app_get_mouse_relative();

void app_set_keep_awake(bool);

void app_input_init();

void app_input_deinit();

void app_input_set_group(lv_group_t *group);

void app_input_push_modal_group(lv_group_t *group);

void app_input_remove_modal_group(lv_group_t *group);

lv_group_t *app_input_get_group();

void app_input_set_button_points(const lv_point_t *points);

void app_set_fullscreen(bool);

void app_open_url(const char *url);

void app_init_locale();

const char *app_get_locale_lang();