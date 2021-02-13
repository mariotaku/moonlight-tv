#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"

#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

static void _mouse_position_map(short raw_x, short raw_y, short raw_width, short raw_height,
                                short *out_x, short *out_y, short *out_width, short *out_height);

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
    {
        LiSendMouseButtonEvent(event->type == SDL_MOUSEBUTTONDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, button);
    }
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
    // TODO https://github.com/mariotaku/moonlight-tv/issues/1
    // TODO https://github.com/mariotaku/moonlight-tv/issues/2
    if (SDL_GetRelativeMouseMode())
    {
        LiSendMouseMoveEvent(event->xrel, event->yrel);
    }
    else
    {
        short x, y, w, h;
        _mouse_position_map(event->x, event->y, streaming_display_width, streaming_display_height, &x, &y, &w, &h);
        LiSendMousePositionEvent(x, y, w, h);
    }
}

void _mouse_position_map(short raw_x, short raw_y, short raw_width, short raw_height,
                         short *out_x, short *out_y, short *out_width, short *out_height)
{
    short screen_x = 0, screen_y = 193, screen_width = 2560, screen_height = 1440;
    short area_width = 3640, area_height = 1920;
    *out_width = area_width;
    *out_height = area_height;
    *out_x = screen_x + raw_x / (raw_width / (float)screen_width);
    *out_y = screen_y + raw_y / (raw_height / (float)screen_height);
}