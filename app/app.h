#pragma once

#include "lvgl.h"
#include "lvgl/lv_obj_controller.h"
#include "stream/module/api.h"
#include "stream/settings.h"
#include "backend/pcmanager.h"
#include "libgamestream/client.h"

typedef void *APP_WINDOW_CONTEXT;

extern FILE *app_logfile;
extern PCONFIGURATION app_configuration;
extern int app_window_width, app_window_height;
extern lv_controller_manager_t *app_uimanager;
extern lv_indev_t *app_indev_key;
extern pcmanager_t *pcmanager;

int app_init(int argc, char *argv[]);

APP_WINDOW_CONTEXT app_window_create();

void app_destroy();

void app_process_events();

void app_request_exit();

void app_start_text_input(int x, int y, int w, int h);

void app_stop_text_input();

bool ui_render_queue_submit(void *data, unsigned int pts);

GS_CLIENT app_gs_client_new();

void app_set_mouse_grab(bool);

void app_set_keep_awake(bool);