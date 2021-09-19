#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"
#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#include "util/bus.h"
#include "util/user_event.h"

bool webos_intercept_remote_keys(SDL_KeyboardEvent *event, short *keyCode)
{
    // TODO Keyboard event on webOS is incorrect
    // https://github.com/mariotaku/moonlight-sdl/issues/4
    switch ((unsigned int)event->keysym.scancode)
    {
    case SDL_WEBOS_SCANCODE_EXIT:
    {
        if (event->state == SDL_PRESSED)
        {
            bus_pushevent(USER_OPEN_OVERLAY, NULL, NULL);
        }
        return true;
    }
    case SDL_WEBOS_SCANCODE_BACK:
        *keyCode = 0x1B /* SDL_SCANCODE_ESCAPE */;
        return false;
    case SDL_WEBOS_SCANCODE_CH_UP:
        *keyCode = 0x21 /* SDL_SCANCODE_PAGEUP */;
        return false;
    case SDL_WEBOS_SCANCODE_CH_DOWN:
        *keyCode = 0x22 /* SDL_SCANCODE_PAGEDOWN */;
        return false;
    case SDL_WEBOS_SCANCODE_YELLOW:
        if (!absinput_no_control)
        {
            LiSendMouseButtonEvent(event->type == SDL_KEYDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE,
                                   BUTTON_RIGHT);
        }
        return true;
    default:
        return false;
    }
}