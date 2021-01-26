#pragma once

typedef enum NAVKEY_
{
    NAVKEY_UNKNOWN = -1,
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