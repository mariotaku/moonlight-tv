#pragma once

#include <stdbool.h>

#include <SDL.h>

typedef struct _GAMEPAD_STATE {
    char leftTrigger, rightTrigger;
    short leftStickX, leftStickY;
    short rightStickX, rightStickY;
    short buttons;
    SDL_JoystickID sdl_id;
    SDL_Haptic *haptic;
    int haptic_effect_id;
    short id;
    bool initialized;
} GAMEPAD_STATE, *PGAMEPAD_STATE;

extern GAMEPAD_STATE gamepads[4];
extern short activeGamepadMask;

bool absinput_dispatch_event(SDL_Event *event);

bool absinput_should_accept();

bool absinput_init_gamepad(int which);

void absinput_close_gamepad(SDL_JoystickID sdl_id);

bool absinput_gamepad_known(SDL_JoystickID sdl_id);

PGAMEPAD_STATE get_gamepad(SDL_JoystickID sdl_id);

