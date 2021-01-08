#pragma once

#include <stdbool.h>
#include <Limelight.h>

extern bool absinput_no_control;

void absinput_init();

int absinput_gamepads();

ConnListenerRumble absinput_getrumble();

#ifdef HAVE_SDL
#include <SDL.h>

bool absinput_dispatch_event(SDL_Event ev);

#endif