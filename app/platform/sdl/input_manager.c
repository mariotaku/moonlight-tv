#include "backend/input_manager.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"

#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

#include "logging.h"

#include "backend/gamecontrollerdb_updater.h"
#include "config.h"

void inputmgr_init() {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    int numofmappings = 0;

    char *condb = gamecontrollerdb_path();
    commons_log_info("Input", "Load game controller mapping from %s", condb);
    if (access(condb, F_OK) == 0) {
        numofmappings = SDL_GameControllerAddMappingsFromFile(condb);
    }
    free(condb);

    char *userdb = gamecontrollerdb_user_path();
    commons_log_info("Input", "Load user game controller mapping from %s", userdb);
    if (access(userdb, F_OK) == 0) {
        numofmappings = SDL_GameControllerAddMappingsFromFile(userdb);
    }
    free(userdb);

    gamecontrollerdb_update();

    commons_log_info("Input", "Input manager init, %d game controller mappings loaded", numofmappings);
    absinput_init();
}

void inputmgr_destroy() {
    absinput_destroy();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

void inputmgr_sdl_handle_event(SDL_Event *ev) {
    if (ev->type == SDL_JOYDEVICEADDED) {
        if (absinput_gamepads() >= absinput_max_gamepads()) {
            // Ignore controllers more than supported
            commons_log_warn("Input", "Too many controllers, ignoring.");
            return;
        }
        absinput_init_gamepad(ev->jdevice.which);
    } else if (ev->type == SDL_JOYDEVICEREMOVED) {
        absinput_close_gamepad(ev->jdevice.which);
    } else if (ev->type == SDL_CONTROLLERDEVICEADDED) {
        commons_log_debug("Input", "SDL_CONTROLLERDEVICEADDED");
    } else if (ev->type == SDL_CONTROLLERDEVICEREMOVED) {
        commons_log_debug("Input", "SDL_CONTROLLERDEVICEREMOVED");
    } else if (ev->type == SDL_CONTROLLERDEVICEREMAPPED) {
        commons_log_debug("Input", "SDL_CONTROLLERDEVICEREMAPPED");
    }
}