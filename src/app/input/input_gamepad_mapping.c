#include <unistd.h>
#include "input_gamepad_mapping.h"
#include "stream/settings.h"
#include "executor.h"
#include "gamecontrollerdb_updater.h"
#include "util/user_event.h"
#include "util/path.h"
#include "copyfile.h"

static void gcdb_updated(commons_gcdb_status_t result, void *context);

static char *gamecontrollerdb_builtin_path();

void app_input_init_gamepad_mapping(app_input_t *input, executor_t *executor, const app_settings_t *settings) {
    input->gcdb_updater.callback = gcdb_updated;
    input->gcdb_updater.path = settings->condb_path;
    input->gcdb_updater.platform = GAMECONTROLLERDB_PLATFORM;
#ifdef GAMECONTROLLERDB_PLATFORM_USE
    input->gcdb_updater.platform_use = GAMECONTROLLERDB_PLATFORM_USE;
#endif
    commons_gcdb_updater_init(&input->gcdb_updater, executor);
    commons_gcdb_updater_update(&input->gcdb_updater);
}

void app_input_deinit_gamepad_mapping(app_input_t *input) {
    commons_gcdb_updater_deinit(&input->gcdb_updater);
}

void app_input_copy_initial_gamepad_mapping(const app_settings_t *settings) {
    if (access(settings->condb_path, F_OK) == 0) {
        return;
    }
    char *builtin = gamecontrollerdb_builtin_path();
    copyfile(builtin, settings->condb_path);
    free(builtin);
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
    char *assetsdir = path_assets(), *condb = path_join(assetsdir, "sdl_gamecontrollerdb.txt");
    free(assetsdir);
    return condb;
}