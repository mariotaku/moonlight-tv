#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum NAVKEY_
{
    NAVKEY_UNKNOWN = 0,
    NAVKEY_DPAD = 0x100,
    NAVKEY_UP,
    NAVKEY_DOWN,
    NAVKEY_LEFT,
    NAVKEY_RIGHT,
    NAVKEY_FUNCTION = 0x200,
    NAVKEY_MENU,
    NAVKEY_START,
    NAVKEY_CONFIRM /* A button*/,
    NAVKEY_CANCEL /* B button*/,
    NAVKEY_NEGATIVE /* X button*/,
    NAVKEY_ALTERNATIVE /* Y button*/,
} NAVKEY;

typedef enum NAVKEY_STATE_
{
    NAVKEY_STATE_UP = 0x0,
    NAVKEY_STATE_DOWN = 0x1,
    NAVKEY_STATE_REPEAT = 0x2,
} NAVKEY_STATE;

bool navkey_intercept_repeat(NAVKEY_STATE state, uint32_t timestamp);