#pragma once

#include <stdbool.h>

#include <SDL.h>

bool absinput_dispatch_event(SDL_Event ev);

bool absinput_controllerdevice_event(SDL_Event ev);

bool absinput_init_gamepad(int which);

void absinput_close_gamepad(SDL_JoystickID sdl_id);