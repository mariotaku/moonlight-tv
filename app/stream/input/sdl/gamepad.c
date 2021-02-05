#include "stream/input/sdlinput.h"

#include <Limelight.h>
#include <SDL.h>

#define QUIT_BUTTONS (PLAY_FLAG | BACK_FLAG | LB_FLAG | RB_FLAG)

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
        return;

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
        gamepad->leftStickY = -event->value - 1;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
        gamepad->rightStickX = event->value;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
        gamepad->rightStickY = -event->value - 1;
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        gamepad->leftTrigger = (event->value >> 8) * 2;
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
        gamepad->rightTrigger = (event->value >> 8) * 2;
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
