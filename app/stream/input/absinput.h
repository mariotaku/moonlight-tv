#pragma once

#include <stdbool.h>
#include <Limelight.h>

extern bool absinput_no_control;

void absinput_init();

void absinput_destroy();

int absinput_gamepads();

int absinput_max_gamepads();

bool absinput_gamepad_present(int which);

void absinput_rumble(unsigned short controllerNumber, unsigned short lowFreqMotor, unsigned short highFreqMotor);

void absinput_start();

void absinput_stop();

void absinput_set_virtual_mouse(bool enabled);

bool absinput_get_virtual_mouse();