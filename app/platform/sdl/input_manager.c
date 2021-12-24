#include "backend/input_manager.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"

#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

#include "util/logging.h"

#include "backend/gamecontrollerdb_updater.h"

#define SDL_CONTROLLER_PLATFORM_FIELD   "platform:"

#if TARGET_WEBOS
static int BACKPORT_GameControllerAddMappingsFromFile(const char *path);
#endif

void inputmgr_init() {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    int numofmappings;

    char *condb = gamecontrollerdb_path();
    if (access(condb, F_OK) == 0) {
        numofmappings = SDL_GameControllerAddMappingsFromFile(condb);
    } else {
#if TARGET_WEBOS
        numofmappings = BACKPORT_GameControllerAddMappingsFromFile("assets/gamecontrollerdb.txt");
#else
        numofmappings = SDL_GameControllerAddMappingsFromFile("third_party/SDL_GameControllerDB/gamecontrollerdb.txt");
#endif
    }
    free(condb);

    gamecontrollerdb_update();

    applog_i("Input", "Input manager init, %d game controller mappings loaded", numofmappings);
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

#if TARGET_WEBOS
static int BACKPORT_GameControllerAddMappingsFromFile(const char *path) {
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    const char *platform = SDL_GetPlatform();
    int controllers = 0;
    char *buf, *line, *line_end, *tmp, *comma, line_platform[64];
    size_t db_size, platform_len;

    if (rw == NULL) {
        return SDL_SetError("Invalid RWops");
    }
    db_size = (size_t) SDL_RWsize(rw);

    buf = (char *) SDL_malloc(db_size + 1);
    if (buf == NULL) {
        SDL_RWclose(rw);
        return SDL_SetError("Could not allocate space to read DB into memory");
    }

    if (SDL_RWread(rw, buf, db_size, 1) != 1) {
        SDL_RWclose(rw);
        SDL_free(buf);
        return SDL_SetError("Could not read DB");
    }

    SDL_RWclose(rw);

    buf[db_size] = '\0';
    line = buf;

    while (line < buf + db_size) {
        line_end = SDL_strchr(line, '\n');
        if (line_end != NULL) {
            *line_end = '\0';
        } else {
            line_end = buf + db_size;
        }

        /* Extract and verify the platform */
        tmp = SDL_strstr(line, SDL_CONTROLLER_PLATFORM_FIELD);
        if (tmp != NULL) {
            tmp += SDL_strlen(SDL_CONTROLLER_PLATFORM_FIELD);
            comma = SDL_strchr(tmp, ',');
            if (comma != NULL) {
                platform_len = comma - tmp + 1;
                if (platform_len + 1 < SDL_arraysize(line_platform)) {
                    SDL_strlcpy(line_platform, tmp, platform_len);
                    if (SDL_strncasecmp(line_platform, platform, platform_len) == 0 &&
                        SDL_GameControllerAddMapping(line) > 0) {
                        controllers++;
                    }
                }
            }
        }

        line = line_end + 1;
    }

    SDL_free(buf);
    return controllers;
}
#endif