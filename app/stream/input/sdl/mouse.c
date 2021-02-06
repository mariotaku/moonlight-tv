#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"

#include <Limelight.h>
#include <SDL.h>

void sdlinput_handle_mbutton_event(SDL_MouseButtonEvent *event)
{
    if (absinput_no_control)
        return;
    int button = 0;
    switch (event->button)
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
    default:
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Unhandled button event: %d",
                    event->button);
        return;
    }

    if (button != 0)
        LiSendMouseButtonEvent(event->type == SDL_MOUSEBUTTONDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, button);
}

void sdlinput_handle_mwheel_event(SDL_MouseWheelEvent *event)
{
    if (absinput_no_control)
        return;
    LiSendScrollEvent(event->y);
}

void sdlinput_handle_mmotion_event(SDL_MouseMotionEvent *event)
{
    if (absinput_no_control)
        return;
    LiSendMouseMoveEvent(event->xrel, event->yrel);
}