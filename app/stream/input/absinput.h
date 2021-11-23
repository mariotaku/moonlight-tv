#pragma once

#include <stdbool.h>
#include <Limelight.h>

typedef enum absinput_vmouse_mode_t {
    ABSINPUT_VMOUSE_OFF = 0x0,
    ABSINPUT_VMOUSE_LEFT_STICK = 0x1,
    ABSINPUT_VMOUSE_RIGHT_STICK = 0x2,
} absinput_vmouse_mode_t;

extern bool absinput_no_control;

extern absinput_vmouse_mode_t absinput_virtual_mouse;

void absinput_init();

void absinput_destroy();

int absinput_gamepads();

int absinput_max_gamepads();

bool absinput_gamepad_present(int which);

void absinput_rumble(unsigned short controllerNumber, unsigned short lowFreqMotor, unsigned short highFreqMotor);