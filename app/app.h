#pragma once

#include "stream/settings.h"

typedef void *APP_WINDOW_CONTEXT;

PCONFIGURATION app_configuration;

int app_init(int argc, char *argv[]);

APP_WINDOW_CONTEXT app_window_create();

void app_destroy();

void app_main_loop(void *data);