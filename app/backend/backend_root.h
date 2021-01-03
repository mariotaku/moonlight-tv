#pragma once

#include <stdbool.h>

#include <SDL.h>

void backend_init();

void backend_destroy();

bool backend_dispatch_userevent(SDL_Event ev);