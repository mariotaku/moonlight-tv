#include <stdlib.h>

#include <lgnc_system.h>

#include "absinput.h"
#include "lgncinput.h"

bool absinput_no_control = true;

void absinput_init()
{
}

void absinput_destroy()
{
}

int absinput_gamepads()
{
    return 0;
}

int absinput_max_gamepads()
{
    return 4;
}

bool absinput_gamepad_present(int which)
{
    return false;
}

void absinput_rumble(unsigned short controller_id, unsigned short low_freq_motor, unsigned short high_freq_motor)
{
}

void absinput_dispatch_mouse_event(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond, struct input_event raw)
{
    switch (keyCond)
    {
    case LGNC_KEY_PRESS:
    case LGNC_KEY_RELEASE:
    {
        /* code */
        if (key != 272)
        {
            break;
        }
        char action = keyCond == LGNC_KEY_PRESS ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE;
        LiSendMouseButtonEvent(action, BUTTON_LEFT);
        break;
    }
    case LGNC_KEY_COND_LAST:
    {
        switch (raw.type)
        {
        case EV_REL:
            switch (raw.code)
            {
            case REL_WHEEL:
                LiSendScrollEvent(raw.value);
                break;
            default:
                break;
            }
            break;
        case EV_ABS:
            switch (raw.code)
            {
            case ABS_X:
            case ABS_Y:
                LiSendMousePositionEvent(posX, posY, 1280, 720);
                break;
            }
            break;
        default:
            break;
        }
        break;
    }
    }
}