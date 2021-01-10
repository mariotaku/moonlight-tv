#include <stdlib.h>

#include <lgnc_system.h>

#include "absinput.h"
#include "lgnc.h"

bool absinput_no_control = true;

void absinput_init()
{
}

int absinput_gamepads()
{
    return 0;
}

ConnListenerRumble absinput_getrumble()
{
    return NULL;
}

void absinput_dispatch_mouse_event(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond)
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
        if (key != 0)
        {
            break;
        }
        LiSendMousePositionEvent(posX, posY, 1280, 720);
        break;
    }
    }
}