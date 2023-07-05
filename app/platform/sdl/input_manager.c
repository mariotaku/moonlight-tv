#include "backend/input_manager.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"

#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

#include "logging.h"

#include "gamecontrollerdb_updater.h"
#include "config.h"
#include "util/path.h"

void inputmgr_init(input_manager_t *manager) {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    int numofmappings = 0;

    char *condb = gamecontrollerdb_fetched_path();
    if (access(condb, F_OK) == 0) {
        commons_log_info("Input", "Load game controller mapping from %s", condb);
        numofmappings += SDL_GameControllerAddMappingsFromFile(condb);
    } else {
        char *builtin = gamecontrollerdb_builtin_path();
        if (access(builtin, F_OK) == 0) {
            commons_log_info("Input", "Load builtin game controller mapping from %s", builtin);
            numofmappings += SDL_GameControllerAddMappingsFromFile(builtin);
        }
        free(builtin);
    }

    char *extradb = gamecontrollerdb_extra_path();
    if (access(extradb, F_OK) == 0) {
        commons_log_info("Input", "Load extra controller mapping from %s", extradb);
        numofmappings += SDL_GameControllerAddMappingsFromFile(extradb);
    }
    free(extradb);

    char *userdb = gamecontrollerdb_user_path();
    if (access(userdb, F_OK) == 0) {
        commons_log_info("Input", "Load user game controller mapping from %s", userdb);
        numofmappings += SDL_GameControllerAddMappingsFromFile(userdb);
    }
    free(userdb);

    commons_log_info("Input", "Input manager init, %d game controller mappings loaded", numofmappings);

    manager->gcdb_updater.path = condb;
    manager->gcdb_updater.platform = GAMECONTROLLERDB_PLATFORM;
#ifdef GAMECONTROLLERDB_PLATFORM_USE
    manager->gcdb_updater.platform_use = GAMECONTROLLERDB_PLATFORM_USE;
#endif

    commons_gcdb_updater_init(&manager->gcdb_updater);
    commons_gcdb_updater_update(&manager->gcdb_updater);
}

void inputmgr_deinit(input_manager_t *manager) {
    free(manager->gcdb_updater.path);
    commons_gcdb_updater_deinit(&manager->gcdb_updater);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

char *gamecontrollerdb_builtin_path() {
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "sdl_gamecontrollerdb.txt");
    free(assetsdir);
    return condb;
}

char *gamecontrollerdb_extra_path() {
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "sdl_gamecontrollerdb_extra.txt");
    free(assetsdir);
    return condb;
}

char *gamecontrollerdb_fetched_path() {
    char *confdir = path_pref(), *condb = path_join(confdir, "sdl_gamecontrollerdb.txt");
    free(confdir);
    return condb;
}

char *gamecontrollerdb_user_path() {
    char *confdir = path_pref(), *condb = path_join(confdir, "sdl_gamecontrollerdb_user.txt");
    free(confdir);
    return condb;
}
