#include "stream/input/sdlinput.h"
#include "stream/input/absinput.h"
#include "stream/session.h"

#include <Limelight.h>
#include <SDL.h>

#include "platform/sdl/webos_keys.h"

#include "util/bus.h"
#include "util/user_event.h"

bool webos_intercept_remote_keys(SDL_KeyboardEvent *event)
{
    // TODO Keyboard event on webOS is incorrect
    // https://github.com/mariotaku/moonlight-sdl/issues/4
    switch (event->keysym.scancode)
    {
    case SDL_WEBOS_SCANCODE_BACK:
    {
        if (event->state == SDL_RELEASED)
        {
            bus_pushevent(USER_ST_QUITAPP_CONFIRM, NULL, NULL);
        }
        return true;
    }
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