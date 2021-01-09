#include "absinput.h"

// Include source directly in order to use static functions
#include "input/sdl.c"
#include "stream/session.h"
#include "util/user_event.h"

static void release_gamecontroller_buttons(int which);
static void release_keyboard_keys(SDL_Event ev);

bool absinput_no_control;

void absinput_init()
{
    sdlinput_init("assets/gamecontrollerdb.txt");
}

int absinput_gamepads()
{
    return sdl_gamepads;
}

ConnListenerRumble absinput_getrumble()
{
    return sdlinput_rumble;
}

static bool nocontrol_handle_event(SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        int modifier = 0;
        switch (ev.key.keysym.sym)
        {
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            modifier = MODIFIER_SHIFT;
            break;
        case SDLK_RALT:
        case SDLK_LALT:
            modifier = MODIFIER_ALT;
            break;
        case SDLK_RCTRL:
        case SDLK_LCTRL:
            modifier = MODIFIER_CTRL;
            break;
        }

        if (modifier != 0)
        {
            if (ev.type == SDL_KEYDOWN)
            {
                keyboard_modifiers |= modifier;
            }
            else
            {
                keyboard_modifiers &= ~modifier;
            }
        }

        // Quit the stream if all the required quit keys are down
        if ((keyboard_modifiers & ACTION_MODIFIERS) == ACTION_MODIFIERS && ev.key.keysym.sym == QUIT_KEY && ev.type == SDL_KEYUP)
        {
            return SDL_QUIT_APPLICATION;
        }
        else if ((keyboard_modifiers & ACTION_MODIFIERS) == ACTION_MODIFIERS && ev.key.keysym.sym == FULLSCREEN_KEY && ev.type == SDL_KEYUP)
        {
            return SDL_TOGGLE_FULLSCREEN;
        }
        else if ((keyboard_modifiers & ACTION_MODIFIERS) == ACTION_MODIFIERS)
        {
            return SDL_MOUSE_UNGRAB;
        }
        break;
    }
    default:
        break;
    }
    return SDL_NOTHING;
}

bool absinput_dispatch_event(SDL_Event ev)
{
    if (streaming_status != STREAMING_STREAMING)
    {
        return false;
    }
    // TODO Keyboard event on webOS is incorrect
    // https://github.com/mariotaku/moonlight-sdl/issues/4
#if OS_WEBOS
    if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP)
    {
        return false;
    }
#endif
    // Don't mess with Magic Remote yet
    // TODO https://github.com/mariotaku/moonlight-sdl/issues/1
    // TODO https://github.com/mariotaku/moonlight-sdl/issues/2
    if (!absinput_no_control && ev.type == SDL_MOUSEMOTION)
    {
        if (ev.motion.state == SDL_PRESSED)
        {
            LiSendMouseMoveEvent(ev.motion.xrel, ev.motion.yrel);
        }
        else
        {
            LiSendMousePositionEvent(ev.motion.x, ev.motion.y, streaming_display_width, streaming_display_height);
        }
        return false;
    }
    switch (absinput_no_control ? nocontrol_handle_event(ev) : sdlinput_handle_event(&ev))
    {
    case SDL_MOUSE_GRAB:
        break;
    case SDL_MOUSE_UNGRAB:
        break;
    case SDL_QUIT_APPLICATION:
    {
        switch (ev.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            release_keyboard_keys(ev);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            // Put gamepad to neutral state
            release_gamecontroller_buttons(ev.cbutton.which);
            break;
        }

        SDL_Event quitapp;
        quitapp.type = SDL_USEREVENT;
        quitapp.user.code = USER_ST_QUITAPP_CONFIRM;
        SDL_PushEvent(&quitapp);
        break;
    }
    default:
        break;
    }
    return false;
}

void release_gamecontroller_buttons(int which)
{
    PGAMEPAD_STATE gamepad;
    gamepad = get_gamepad(which);
    gamepad->buttons = 0;
    gamepad->leftTrigger = 0;
    gamepad->rightTrigger = 0;
    gamepad->leftStickX = 0;
    gamepad->leftStickY = 0;
    gamepad->rightStickX = 0;
    gamepad->rightStickY = 0;
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger, gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
}

void release_keyboard_keys(SDL_Event ev)
{
    keyboard_modifiers = 0;
}