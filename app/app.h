#pragma once

#include "lvgl.h"
#include "ui/manager.h"
#include "stream/module/api.h"
#include "stream/settings.h"
#include "libgamestream/client.h"

typedef void *APP_WINDOW_CONTEXT;

extern FILE *app_logfile;
extern PCONFIGURATION app_configuration;
extern int app_window_width, app_window_height;
extern bool app_has_redraw, app_force_redraw;
extern uimanager_ctx *app_uimanager;
extern lv_indev_t *app_indev_key;

int app_init(int argc, char *argv[]);

APP_WINDOW_CONTEXT app_window_create();

void app_destroy();

void app_process_events();

void app_request_exit();

void app_start_text_input(int x, int y, int w, int h);

void app_stop_text_input();

bool app_render_queue_submit(void *);

GS_CLIENT app_gs_client_obtain();

bool app_gs_client_ready();

void app_set_mouse_grab(bool);

void app_set_keep_awake(bool);