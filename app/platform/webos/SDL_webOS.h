#pragma once

#include <SDL_types.h>

#define SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK "SDL_WEBOS_ACCESS_POLICY_KEYS_BACK"
#define SDL_HINT_WEBOS_CURSOR_SLEEP_TIME "SDL_WEBOS_CURSOR_SLEEP_TIME"

SDL_bool SDL_webOSCursorVisibility(SDL_bool visible);

SDL_bool SDL_webOSGetPanelResolution(int *width,int *height);

SDL_bool SDL_webOSGetRefreshRate(int *rate);