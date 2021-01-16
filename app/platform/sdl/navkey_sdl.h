#pragma once

#include <SDL2/SDL.h>
#include "util/navkey.h"

NAVKEY navkey_from_sdl(SDL_Event ev);