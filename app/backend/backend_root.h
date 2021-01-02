#pragma once

#include <stdbool.h>

#include <SDL.h>

void backend_init();

void backend_destroy();

bool backend_dispatch_event(SDL_Event ev);