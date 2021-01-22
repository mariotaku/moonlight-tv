#include "sdlinput.h"
#include "absinput.h"

// Include source directly in order to use static functions
#include "src/input/sdl.c"
#include "stream/session.h"
#include "util/user_event.h"

#if OS_WEBOS
#include "platform/sdl/webos_keys.h"
#endif

static void release_gamecontroller_buttons(int which);
static void release_keyboard_keys(SDL_Event ev);
static void sdlinput_handle_input_result(SDL_Event ev, int ret);
static int _sdlinput_handle_event_fix(SDL_Event *ev);

bool absinput_no_control;

void absinput_init()
{
    memset(gamepads, 0, sizeof(gamepads));
}

void absinput_destroy()
{
}

int absinput_gamepads()
{
    return sdl_gamepads;
}

int absinput_max_gamepads()
{
    return 4;
}

bool absinput_gamepad_present(int which)
{
    return (activeGamepadMask & (1 << which)) != 0;
}

void absinput_rumble(unsigned short controllerNumber, unsigned short lowFreqMotor, unsigned short highFreqMotor)
{
    sdlinput_rumble(controllerNumber, lowFreqMotor, highFreqMotor);
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
    sdlinput_handle_input_result(ev, absinput_no_control ? nocontrol_handle_event(ev) : _sdlinput_handle_event_fix(&ev));
    return false;
}

int _sdlinput_handle_event_fix(SDL_Event *event)
{
    PGAMEPAD_STATE gamepad;
    switch (event->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        // TODO Keyboard event on webOS is incorrect
        // https://github.com/mariotaku/moonlight-sdl/issues/4
#if OS_WEBOS
        switch (event->key.keysym.sym)
        {
        case SDLK_WEBOS_BACK:
            if (event->type == SDL_KEYUP)
            {
                return SDL_QUIT_APPLICATION;
            }
            else
            {
                return SDL_NOTHING;
            }
        case SDLK_WEBOS_YELLOW:
        {
            char action = event->type == SDL_KEYDOWN ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE;
            LiSendMouseButtonEvent(action, BUTTON_RIGHT);
            return SDL_NOTHING;
        }
        default:
            break;
        }
#endif
        break;
    }
    case SDL_CONTROLLERAXISMOTION:
        gamepad = get_gamepad(event->caxis.which);
        switch (event->caxis.axis)
        {
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            gamepad->leftTrigger = (event->caxis.value >> 8) * 2;
            LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger, gamepad->rightTrigger,
                                       gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
            return SDL_NOTHING;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            gamepad->rightTrigger = (event->caxis.value >> 8) * 2;
            LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger, gamepad->rightTrigger,
                                       gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
            return SDL_NOTHING;
        default:
            break;
        }
    default:
        break;
    }
    return sdlinput_handle_event(event);
}

bool absinput_controllerdevice_event(SDL_Event ev)
{
    switch (ev.type)
    {
    case SDL_CONTROLLERDEVICEADDED:
    {
        const char *name = SDL_GameControllerNameForIndex(ev.cdevice.which);
        printf("SDL_CONTROLLERDEVICEADDED: %s(%d) connected\n", name, ev.cdevice.which);
        break;
    }
    case SDL_CONTROLLERDEVICEREMOVED:
    {
        const char *name = SDL_GameControllerNameForIndex(ev.cdevice.which);
        printf("SDL_CONTROLLERDEVICEREMOVED: %s(%d) disconnected\n", name, ev.cdevice.which);
        break;
    }
    case SDL_CONTROLLERDEVICEREMAPPED:
        printf("SDL_CONTROLLERDEVICEREMAPPED, which: %d\n", ev.cdevice.which);
        break;
    default:
        break;
    }
}

void sdlinput_handle_input_result(SDL_Event ev, int ret)
{
    switch (ret)
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

bool absinput_init_gamepad(int joystick_index)
{
    if (SDL_IsGameController(joystick_index))
    {
        sdl_gamepads++;
        SDL_GameController *controller = SDL_GameControllerOpen(joystick_index);
        if (!controller)
        {
            fprintf(stderr, "Could not open gamecontroller %i: %s\n", joystick_index, SDL_GetError());
            return true;
        }

        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
        SDL_Haptic *haptic = SDL_HapticOpenFromJoystick(joystick);
        if (haptic && (SDL_HapticQuery(haptic) & SDL_HAPTIC_LEFTRIGHT) == 0)
        {
            SDL_HapticClose(haptic);
            haptic = NULL;
        }

        SDL_JoystickID sdl_id = SDL_JoystickInstanceID(joystick);
        PGAMEPAD_STATE state = get_gamepad(sdl_id);
        state->haptic = haptic;
        state->haptic_effect_id = -1;
        printf("Controller #%d connected, sdl_id: %d\n", state->id, sdl_id);
        return true;
    }
    return false;
}

void absinput_close_gamepad(SDL_JoystickID sdl_id)
{
    PGAMEPAD_STATE state = get_gamepad(sdl_id);
    if (!state)
    {
        return;
    }
    SDL_GameController *controller = SDL_GameControllerFromInstanceID(sdl_id);
    if (!controller)
    {
        fprintf(stderr, "Could not find gamecontroller %i: %s\n", sdl_id, SDL_GetError());
        return;
    }
    // Reduce number of connected gamepads
    sdl_gamepads--;
    // Remove gamepad mask
    activeGamepadMask &= ~(1 << state->id);
    if (state->haptic)
    {
        SDL_HapticClose(state->haptic);
    }
    SDL_GameControllerClose(controller);
    printf("Controller #%d disconnected, sdl_id: %d\n", state->id, sdl_id);
    // Release the state so it can be reused later
    memset(state, 0, sizeof(GAMEPAD_STATE));
}