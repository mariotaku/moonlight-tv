#include "backend/input_manager.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "events.h"

void inputmgr_init()
{
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
        if (absinput_gamepads() >= absinput_max_gamepads())
        {
            // Ignore controllers more than supported
            return;
        }
        absinput_init_gamepad(ev.jdevice.which);
    }
    else if (ev.type == SDL_JOYDEVICEREMOVED)
    {
        absinput_close_gamepad(ev.jdevice.which);
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
}