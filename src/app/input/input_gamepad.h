#pragma once

#include <stdbool.h>
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>

typedef struct app_input_t app_input_t;
typedef struct app_gamepad_state_t app_gamepad_state_t;

bool app_input_init_gamepad(app_input_t *input, int device_index);

void app_input_close_gamepad(app_input_t *input, SDL_JoystickID sdl_id);

int app_input_get_gamepads_count(app_input_t *input);

short app_input_get_max_gamepads(app_input_t *input);

short app_input_gamepads_mask(app_input_t *input);

void app_input_gamepad_rumble(app_input_t *input, unsigned short controllerNumber, unsigned short lowFreqMotor,
                              unsigned short highFreqMotor);

void app_input_gamepad_rumble_triggers(app_input_t *input, unsigned short controllerNumber, unsigned short leftTrigger,
                                       unsigned short rightTrigger);

void app_input_gamepad_set_motion_event_state(app_input_t *input, unsigned short controllerNumber, uint8_t motionType,
                                              uint16_t reportRateHz);

void app_input_gamepad_set_controller_led(app_input_t *input, unsigned short controllerNumber, uint8_t r, uint8_t g,
                                          uint8_t b);

app_gamepad_state_t * app_input_gamepad_state_init(app_input_t *input, SDL_GameController *controller);
void app_input_gamepad_state_deinit(app_gamepad_state_t *state);

app_gamepad_state_t *app_input_gamepad_state_by_index(app_input_t *input, int index);

app_gamepad_state_t *app_input_gamepad_state_by_instance_id(app_input_t *input, SDL_JoystickID instance_id);