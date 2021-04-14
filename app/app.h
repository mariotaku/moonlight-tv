#pragma once

#include "stream/settings.h"
#include "libgamestream/client.h"

typedef void *APP_WINDOW_CONTEXT;

extern PCONFIGURATION app_configuration;
extern GS_CLIENT app_gs_client;
extern int app_window_width, app_window_height;
extern bool app_has_redraw, app_force_redraw;

int app_init(int argc, char *argv[]);

APP_WINDOW_CONTEXT app_window_create();

void app_destroy();

void app_main_loop(void *data);

void app_request_exit();

void app_start_text_input(int x, int y, int w, int h);

void app_stop_text_input();
