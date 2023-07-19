#include "backend/input_manager.h"
#include "stream/input/absinput.h"

#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

#include "logging.h"

#include "gamecontrollerdb_updater.h"
#include "config.h"
#include "util/path.h"
#include "stream/settings.h"

char *gamecontrollerdb_builtin_path(const app_settings_t *settings);

char *gamecontrollerdb_extra_path(const app_settings_t *settings);

char *gamecontrollerdb_fetched_path(const app_settings_t *settings);

char *gamecontrollerdb_user_path(const app_settings_t *settings);

void input_manager_init(input_manager_t *manager, const app_settings_t *settings) {
    int numofmappings = 0;

    char *condb = gamecontrollerdb_fetched_path(settings);
    if (access(condb, F_OK) == 0) {
        commons_log_info("Input", "Load game controller mapping from %s", condb);
        numofmappings += SDL_GameControllerAddMappingsFromFile(condb);
    } else {
        char *builtin = gamecontrollerdb_builtin_path(settings);
        if (access(builtin, F_OK) == 0) {
            commons_log_info("Input", "Load builtin game controller mapping from %s", builtin);
            numofmappings += SDL_GameControllerAddMappingsFromFile(builtin);
        }
        free(builtin);
    }

    char *extradb = gamecontrollerdb_extra_path(settings);
    if (access(extradb, F_OK) == 0) {
        commons_log_info("Input", "Load extra controller mapping from %s", extradb);
        numofmappings += SDL_GameControllerAddMappingsFromFile(extradb);
    }
    free(extradb);

    char *userdb = gamecontrollerdb_user_path(settings);
    if (access(userdb, F_OK) == 0) {
        commons_log_info("Input", "Load user game controller mapping from %s", userdb);
        numofmappings += SDL_GameControllerAddMappingsFromFile(userdb);
    }
    free(userdb);

    commons_log_info("Input", "Input manager init, %d game controller mappings loaded", numofmappings);


}

char *gamecontrollerdb_builtin_path(const app_settings_t *settings) {
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "sdl_gamecontrollerdb.txt");
    free(assetsdir);
    return condb;
}

char *gamecontrollerdb_extra_path(const app_settings_t *settings) {
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "sdl_gamecontrollerdb_extra.txt");
    free(assetsdir);
    return condb;
}

char *gamecontrollerdb_fetched_path(const app_settings_t *settings) {
    return path_join(settings->conf_dir, "sdl_gamecontrollerdb.txt");
}

char *gamecontrollerdb_user_path(const app_settings_t *settings) {
    return path_join(settings->conf_dir, "sdl_gamecontrollerdb_user.txt");
}
