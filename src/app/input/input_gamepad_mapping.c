#include <unistd.h>
#include <errno.h>
#include "input_gamepad_mapping.h"
#include "app_settings.h"
#include "executor.h"
#include "gamecontrollerdb_updater.h"
#include "util/user_event.h"
#include "util/path.h"
#include "copyfile.h"
#include "logging.h"


static void gcdb_updated(commons_gcdb_status_t result, void *context);

static char *gamecontrollerdb_builtin_path();

static char *gamecontrollerdb_extra_path();

void app_input_init_gamepad_mapping(app_input_t *input, executor_t *executor, const app_settings_t *settings) {
    input->gcdb_updater.callback = gcdb_updated;
    input->gcdb_updater.path = settings->condb_path;
    input->gcdb_updater.platform = GAMECONTROLLERDB_PLATFORM;
#ifdef GAMECONTROLLERDB_PLATFORM_USE
    input->gcdb_updater.platform_use = GAMECONTROLLERDB_PLATFORM_USE;
#endif
    char *condb_extra = gamecontrollerdb_extra_path();
    if (condb_extra != NULL) {
        int num_mapping = SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile(condb_extra, "r"), SDL_TRUE);
        free(condb_extra);
        commons_log_debug("Input", "Added %d gamepad mapping from extra controller db", num_mapping);
    }
    commons_gcdb_updater_init(&input->gcdb_updater, executor);
    commons_gcdb_updater_update(&input->gcdb_updater);
}

void app_input_deinit_gamepad_mapping(app_input_t *input) {
    commons_gcdb_updater_deinit(&input->gcdb_updater);
}

void app_input_copy_initial_gamepad_mapping(const app_settings_t *settings) {
    char *builtin_path = gamecontrollerdb_builtin_path();
    if (builtin_path == NULL) {
        return;
    }
    if (access(settings->condb_path, F_OK) == 0) {
        commons_log_debug("Input", "Skip copying initial controller db file");
    } else if (copyfile(builtin_path, settings->condb_path) == 0) {
        commons_log_info("Input", "Copied initial controller db file");
    } else {
        commons_log_warn("Input", "Failed to copy initial controller db file: %s", strerror(errno));
    }
    free(builtin_path);
}

void app_input_reload_gamepad_mapping(app_input_t *input) {
    int num_mapping = SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile(input->gcdb_updater.path, "r"),
                                                          SDL_TRUE);
    commons_log_debug("Input", "Added %d gamepad mapping", num_mapping);
}

static void gcdb_updated(commons_gcdb_status_t result, void *context) {
    (void) context;
    if (result != COMMONS_GCDB_UPDATER_UPDATED) {
        return;
    }
    SDL_Event event = {
            .user = {
                    .type = SDL_USEREVENT,
                    .code = USER_INPUT_CONTROLLERDB_UPDATED,
            }
    };
    SDL_PushEvent(&event);
}


static char *gamecontrollerdb_builtin_path() {
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "gamecontrollerdb.txt");
    free(assetsdir);
    if (access(condb, R_OK | F_OK) != 0) {
        free(condb);
        return NULL;
    }
    return condb;
}

static char *gamecontrollerdb_extra_path() {
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "gamecontrollerdb_extra.txt");
    free(assetsdir);
    if (access(condb, R_OK | F_OK) != 0) {
        free(condb);
        return NULL;
    }
    return condb;
}