#include "stream/input/sdlinput.h"

#include <Limelight.h>
#include <SDL.h>

#include "util/bus.h"
#include "util/user_event.h"

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)

static bool quit_combo_pressed = false;

void sdlinput_handle_cbutton_event(SDL_ControllerButtonEvent *event)
{
    int button = 0;
    PGAMEPAD_STATE gamepad = get_gamepad(event->which);
    switch (event->button)
    {
    case SDL_CONTROLLER_BUTTON_A:
        button = A_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_B:
        button = B_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_Y:
        button = Y_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_X:
        button = X_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        button = UP_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        button = DOWN_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        button = RIGHT_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        button = LEFT_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_BACK:
        button = BACK_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_START:
        button = PLAY_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_GUIDE:
        button = SPECIAL_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
        button = LS_CLK_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        button = RS_CLK_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        button = LB_FLAG;
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        button = RB_FLAG;
        break;
    default:
        return;
    }
    if (event->type == SDL_CONTROLLERBUTTONDOWN)
        gamepad->buttons |= button;
    else
        gamepad->buttons &= ~button;

    if ((gamepad->buttons & QUIT_BUTTONS) == QUIT_BUTTONS)
    {
        quit_combo_pressed = true;
        return;
    }
    if (quit_combo_pressed)
    {
        if (gamepad->buttons == 0)
        {
            quit_combo_pressed = false;
            bus_pushevent(USER_ST_QUITAPP_CONFIRM, NULL, NULL);
        }
        return;
    }

    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger, gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
}

void sdlinput_handle_caxis_event(SDL_ControllerAxisEvent *event)
{
    PGAMEPAD_STATE gamepad = get_gamepad(event->which);
    switch (event->axis)
    {
    case SDL_CONTROLLER_AXIS_LEFTX:
        gamepad->leftStickX = event->value;
        break;
    case SDL_CONTROLLER_AXIS_LEFTY:
        // Signed values have one more negative value than
        // positive value, so inverting the sign on -32768
        // could actually cause the value to overflow and
        // wrap around to be negative again. Avoid that by
        // capping the value at 32767.
        gamepad->leftStickY = -SDL_max(event->value, (short)-32767);
        break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
        gamepad->rightStickX = event->value;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
        gamepad->rightStickY = -SDL_max(event->value, (short)-32767);
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        gamepad->leftTrigger = (unsigned char)(event->value * 255UL / 32767);
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
        gamepad->rightTrigger = (unsigned char)(event->value * 255UL / 32767);
        break;
    default:
        return;
    }
    LiSendMultiControllerEvent(gamepad->id, activeGamepadMask, gamepad->buttons, gamepad->leftTrigger, gamepad->rightTrigger,
                               gamepad->leftStickX, gamepad->leftStickY, gamepad->rightStickX, gamepad->rightStickY);
}

PGAMEPAD_STATE get_gamepad(SDL_JoystickID sdl_id)
{
    for (int i = 0; i < 4; i++)
    {
        if (!gamepads[i].initialized)
        {
            gamepads[i].sdl_id = sdl_id;
            gamepads[i].id = i;
            gamepads[i].initialized = true;
            activeGamepadMask |= (1 << i);
            return &gamepads[i];
        }
        else if (gamepads[i].sdl_id == sdl_id)
            return &gamepads[i];
    }
    return &gamepads[0];
}
