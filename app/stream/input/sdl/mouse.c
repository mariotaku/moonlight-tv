#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"

#include <Limelight.h>
#include <SDL.h>

void sdlinput_handle_mouse_event(SDL_Event *event)
{
    int button = 0;
    PGAMEPAD_STATE gamepad;
    switch (event->type)
    {
    case SDL_MOUSEMOTION:
        LiSendMouseMoveEvent(event->motion.xrel, event->motion.yrel);
        break;
    case SDL_MOUSEWHEEL:
        LiSendScrollEvent(event->wheel.y);
        break;
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
        switch (event->button.button)
        {
        case SDL_BUTTON_LEFT:
            button = BUTTON_LEFT;
            break;
        case SDL_BUTTON_MIDDLE:
            button = BUTTON_MIDDLE;
            break;
        case SDL_BUTTON_RIGHT:
            button = BUTTON_RIGHT;
            break;
        case SDL_BUTTON_X1:
            button = BUTTON_X1;
            break;
        case SDL_BUTTON_X2:
            button = BUTTON_X2;
            break;
        }

        if (button != 0)
            LiSendMouseButtonEvent(event->type == SDL_MOUSEBUTTONDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, button);

        return;
    }
    return;
}
