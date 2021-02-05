#pragma once

#include <stdbool.h>

#include <SDL.h>

extern bool app_webos_ndl;
extern bool app_webos_lgnc;

int app_webos_init(int argc, char *argv[]);

void app_webos_window_setup(SDL_Window *window);

void app_webos_destroy();