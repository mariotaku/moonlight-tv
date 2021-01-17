#include "backend/input_manager.h"
#include "stream/input/absinput.h"

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "events.h"

static void _inputmgr_joystick_added(int which);
static void _inputmgr_joystick_removed(int instance_id);

struct CONTROLLER_STATE_T
{
    SDL_GameController *controller;
    SDL_Haptic *haptic;
    SDL_JoystickID instanceid;
};

#define MAX_CONTROLLERS 4
static struct CONTROLLER_STATE_T _connected_controllers[MAX_CONTROLLERS];
static int _controller_count;

void inputmgr_init()
{
    _controller_count = 0;
    memset(_connected_controllers, 0, sizeof(_connected_controllers));
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    int numofmappings;
#if OS_WEBOS
    numofmappings = SDL_GameControllerAddMappingsFromFile("assets/gamecontrollerdb.txt");
#else
    numofmappings = SDL_GameControllerAddMappingsFromFile("third_party/SDL_GameControllerDB/gamecontrollerdb.txt");
#endif
    printf("Input manager init, %d game controller mappings loaded\n", numofmappings);
    absinput_init();
}

void inputmgr_destroy()
{
    absinput_destroy();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

void inputmgr_sdl_handle_event(SDL_Event ev)
{
    if (ev.type == SDL_JOYDEVICEADDED)
    {
        _inputmgr_joystick_added(ev.jdevice.which);
    }
    else if (ev.type == SDL_JOYDEVICEREMOVED)
    {
        _inputmgr_joystick_removed(ev.jdevice.which);
    }
    else if (ev.type == SDL_CONTROLLERDEVICEADDED)
    {
        printf("SDL_CONTROLLERDEVICEADDED\n");
    }
    else if (ev.type == SDL_CONTROLLERDEVICEREMOVED)
    {
        printf("SDL_CONTROLLERDEVICEREMOVED\n");
    }
    else if (ev.type == SDL_CONTROLLERDEVICEREMAPPED)
    {
        printf("SDL_CONTROLLERDEVICEREMAPPED\n");
    }
    else if (SDL_IS_GAMECONTROLLER_EVENT(ev))
    {
        switch (ev.type)
        {
        case SDL_CONTROLLERBUTTONDOWN:
            printf("SDL_CONTROLLERBUTTONDOWN, which=%d, key=%d\n", ev.cbutton.which, ev.cbutton.button);
            break;
        case SDL_CONTROLLERAXISMOTION:
            printf("SDL_CONTROLLERAXISMOTION, which=%d, value=%d\n", ev.caxis.which, ev.caxis.value);
            break;
        default:
            printf("Unhandled game controller event 0x%03x\n", ev.type);
            break;
        }
    }
}

void _inputmgr_joystick_added(int which)
{
    printf("SDL_JOYDEVICEADDED\n");
    SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(which);
    static char guidstr[33];
    SDL_JoystickGetGUIDString(guid, guidstr, 33);
    if (!SDL_IsGameController(which))
    {
        fprintf(stderr, "Unrecognized joystick %s\n", guidstr);
        return;
    }
    SDL_GameController *controller = SDL_GameControllerOpen(which);
    if (!controller)
    {
        fprintf(stderr, "Unable to open game controller %s\n", guidstr);
        return;
    }

    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
    SDL_Haptic *haptic = SDL_HapticOpenFromJoystick(joystick);
    if (haptic && (SDL_HapticQuery(haptic) & SDL_HAPTIC_LEFTRIGHT) == 0)
    {
        printf("Game controller %s doesn't support haptic, ignore\n", guidstr);
        SDL_HapticClose(haptic);
        haptic = NULL;
    }
    SDL_JoystickID instanceid = SDL_JoystickInstanceID(joystick);

    const char *name = SDL_GameControllerName(controller);
    int conidx;
    // Find available null controller slot
    for (conidx = 0; conidx < MAX_CONTROLLERS; conidx++)
    {
        if (_connected_controllers[conidx].controller == NULL)
        {
            break;
        }
    }
    // Insert the controller into the first available slot
    if (conidx < MAX_CONTROLLERS)
    {
        _connected_controllers[conidx].instanceid = instanceid;
        _connected_controllers[conidx].controller = controller;
        _connected_controllers[conidx].haptic = haptic;
        printf("Game controller %s connected as #%d. GUID: %s\n", name, conidx + 1, guidstr);
    }
}

void _inputmgr_joystick_removed(int instanceid)
{
    // SDL_GameControllerClose();
    int conidx;
    for (conidx = 0; conidx < MAX_CONTROLLERS; conidx++)
    {
        if (_connected_controllers[conidx].instanceid == instanceid)
        {
            break;
        }
    }
    if (conidx >= MAX_CONTROLLERS)
    {
        return;
    }
    SDL_HapticClose(_connected_controllers[conidx].haptic);
    SDL_GameControllerClose(_connected_controllers[conidx].controller);
    _connected_controllers[conidx].haptic = NULL;
    _connected_controllers[conidx].controller = NULL;
    printf("Joystick %d removed\n", instanceid);
}