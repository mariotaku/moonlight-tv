#pragma once

#include "stream/settings.h"

typedef void *APP_WINDOW_CONTEXT;

PCONFIGURATION app_configuration;

int app_window_width, app_window_height;

bool app_has_redraw, app_force_redraw;

int app_init(int argc, char *argv[]);

APP_WINDOW_CONTEXT app_window_create();

void app_destroy();

void app_main_loop(void *data);

void app_request_exit();

void app_start_text_input(int x, int y, int w, int h);

void app_stop_text_input();
