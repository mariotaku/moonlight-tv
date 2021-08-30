#pragma once

#include <SDL.h>
#include "util/navkey.h"

NAVKEY navkey_from_sdl(const SDL_Event *ev);
