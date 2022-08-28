#include "backend/input_manager.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"

#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

#include "util/logging.h"

#include "backend/gamecontrollerdb_updater.h"
#include "config.h"

#if FEATURE_INPUT_EVMOUSE

#include "platform/linux/evmouse.h"

#endif

void inputmgr_init() {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    int numofmappings = 0;

    char *condb = gamecontrollerdb_path();
    applog_i("Input", "Load game controller mapping from %s", condb);
    if (access(condb, F_OK) == 0) {
        numofmappings = SDL_GameControllerAddMappingsFromFile(condb);
    }
    free(condb);

    char *userdb = gamecontrollerdb_user_path();
    applog_i("Input", "Load user game controller mapping from %s", userdb);
    if (access(userdb, F_OK) == 0) {
        numofmappings = SDL_GameControllerAddMappingsFromFile(userdb);
    }
    free(userdb);

    gamecontrollerdb_update();

    applog_i("Input", "Input manager init, %d game controller mappings loaded", numofmappings);
    absinput_init();

#if FEATURE_INPUT_EVMOUSE
    evmouse_t *mouse = evmouse_open_default();
    if (mouse != NULL) {
        evmouse_close(mouse);
    }
#endif
}

void inputmgr_destroy() {
    absinput_destroy();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

void inputmgr_sdl_handle_event(SDL_Event *ev) {
    if (ev->type == SDL_JOYDEVICEADDED) {
        if (absinput_gamepads() >= absinput_max_gamepads()) {
            // Ignore controllers more than supported
            applog_w("Input", "Too many controllers, ignoring.");
            return;
        }
        absinput_init_gamepad(ev->jdevice.which);
    } else if (ev->type == SDL_JOYDEVICEREMOVED) {
        absinput_close_gamepad(ev->jdevice.which);
    } else if (ev->type == SDL_CONTROLLERDEVICEADDED) {
        applog_d("Input", "SDL_CONTROLLERDEVICEADDED");
    } else if (ev->type == SDL_CONTROLLERDEVICEREMOVED) {
        applog_d("Input", "SDL_CONTROLLERDEVICEREMOVED");
    } else if (ev->type == SDL_CONTROLLERDEVICEREMAPPED) {
        applog_d("Input", "SDL_CONTROLLERDEVICEREMAPPED");
    }
}