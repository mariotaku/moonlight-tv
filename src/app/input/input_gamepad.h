#pragma once

#include <stdbool.h>
#include <SDL_joystick.h>

typedef struct app_input_t app_input_t;
typedef struct app_gamepad_state_t app_gamepad_state_t;

bool app_input_init_gamepad(app_input_t *input, int which);

void app_input_close_gamepad(app_input_t *input, SDL_JoystickID sdl_id);

bool absinput_gamepad_known(app_input_t *input, SDL_JoystickID sdl_id);

int app_input_gamepads(app_input_t *input);

int app_input_max_gamepads(app_input_t *input);

int app_input_gamepads_mask(app_input_t *input);

bool app_input_gamepad_present(app_input_t *input, int which);

void app_input_gamepad_rumble(app_input_t *input, unsigned short controllerNumber, unsigned short lowFreqMotor,
                              unsigned short highFreqMotor);

app_gamepad_state_t *app_input_gamepad_get(app_input_t *input, SDL_JoystickID sdl_id);