#include "sdlinput.h"
#include "absinput.h"

#include <string.h>

#include "stream/session.h"
#include "util/user_event.h"

#if OS_WEBOS
#include "platform/sdl/webos_keys.h"
#endif

static void release_gamecontroller_buttons(int which);
static void release_keyboard_keys(SDL_Event ev);
static void sdlinput_handle_input_result(SDL_Event ev, int ret);

void sdlinput_handle_key_event(SDL_KeyboardEvent *event);
void sdlinput_handle_cbutton_event(SDL_ControllerButtonEvent *event);
void sdlinput_handle_caxis_event(SDL_ControllerAxisEvent *event);
void sdlinput_handle_mbutton_event(SDL_MouseButtonEvent *event);
void sdlinput_handle_mwheel_event(SDL_MouseWheelEvent *event);
void sdlinput_handle_mmotion_event(SDL_MouseMotionEvent *event);

bool absinput_no_control;

GAMEPAD_STATE gamepads[4];
int activeGamepadMask = 0;
int sdl_gamepads = 0;

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

void absinput_rumble(unsigned short controller_id, unsigned short low_freq_motor, unsigned short high_freq_motor)
{
    if (controller_id >= 4)
        return;

    PGAMEPAD_STATE state = &gamepads[controller_id];

    SDL_Haptic *haptic = state->haptic;
    if (!haptic)
        return;

    if (state->haptic_effect_id >= 0)
        SDL_HapticDestroyEffect(haptic, state->haptic_effect_id);

    if (low_freq_motor == 0 && high_freq_motor == 0)
        return;

    SDL_HapticEffect effect;
    SDL_memset(&effect, 0, sizeof(effect));
    effect.type = SDL_HAPTIC_LEFTRIGHT;
    effect.leftright.length = SDL_HAPTIC_INFINITY;

    // SDL haptics range from 0-32767 but XInput uses 0-65535, so divide by 2 to correct for SDL's scaling
    effect.leftright.large_magnitude = low_freq_motor / 2;
    effect.leftright.small_magnitude = high_freq_motor / 2;

    state->haptic_effect_id = SDL_HapticNewEffect(haptic, &effect);
    if (state->haptic_effect_id >= 0)
        SDL_HapticRunEffect(haptic, state->haptic_effect_id, 1);
}

void print_bytes(void *ptr, int size)
{
    unsigned char *p = ptr;
    int i;
    for (i = 0; i < size; i++)
    {
        printf("%02hhX ", p[i]);
    }
    printf("\n");
}

bool absinput_dispatch_event(SDL_Event ev)
{
    if (streaming_status != STREAMING_STREAMING)
    {
        return false;
    }
    switch (ev.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        sdlinput_handle_key_event(&ev.key);
        break;
    case SDL_CONTROLLERAXISMOTION:
        sdlinput_handle_caxis_event(&ev.caxis);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        sdlinput_handle_cbutton_event(&ev.cbutton);
        break;
    case SDL_MOUSEMOTION:
        sdlinput_handle_mmotion_event(&ev.motion);
        break;
    case SDL_MOUSEWHEEL:
        sdlinput_handle_mwheel_event(&ev.wheel);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        sdlinput_handle_mbutton_event(&ev.button);
        break;
    }
    return false;
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