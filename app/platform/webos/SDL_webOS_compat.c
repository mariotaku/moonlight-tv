#include <SDL_webOS.h>
#include <SDL_gamecontroller.h>

#include <dlfcn.h>

#define SDL_BACKPORT __attribute__((unused))
#define SDL_CONTROLLER_PLATFORM_FIELD   "platform:"

static int noop() {
    return 0;
}

SDL_BACKPORT SDL_bool SDL_webOSCursorVisibility(SDL_bool visible) {
    SDL_bool (*fn)(SDL_bool) = (void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "SDL_webOSCursorVisibility");
    }
    if (!fn)
        return SDL_FALSE;
    return fn(visible);
}

SDL_BACKPORT SDL_bool SDL_webOSGetPanelResolution(int *width, int *height) {
    SDL_bool (*fn)(int *, int *) =(void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "SDL_webOSGetPanelResolution");
    }
    if (!fn)
        return SDL_FALSE;
    return fn(width, height);
}

SDL_BACKPORT SDL_bool SDL_webOSGetRefreshRate(int *rate) {
    SDL_bool (*fn)(int *) =(void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "SDL_webOSGetRefreshRate");
    }
    if (!fn)
        return SDL_FALSE;
    return fn(rate);
}

SDL_BACKPORT int SDL_GameControllerAddMappingsFromRW(SDL_RWops *rw, int freerw) {
    const char *platform = SDL_GetPlatform();
    int controllers = 0;
    char *buf, *line, *line_end, *tmp, *comma, line_platform[64];
    size_t db_size, platform_len;

    if (rw == NULL) {
        return SDL_SetError("Invalid RWops");
    }
    db_size = (size_t) rw->size(rw);

    buf = (char *) SDL_malloc(db_size + 1);
    if (buf == NULL) {
        if (freerw) {
            rw->close(rw);
        }
        return SDL_SetError("Could not allocate space to read DB into memory");
    }

    if (rw->read(rw, buf, db_size, 1) != 1) {
        if (freerw) {
            rw->close(rw);
        }
        SDL_free(buf);
        return SDL_SetError("Could not read DB");
    }

    if (freerw) {
        rw->close(rw);
    }

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