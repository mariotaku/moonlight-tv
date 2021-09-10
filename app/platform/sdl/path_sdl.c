#include "util/path.h"

#include <SDL.h>

char *path_pref() {
    char *path = SDL_GetPrefPath("com.limelight", "moonlight-tv");
    unsigned int len = SDL_strlen(path);
    if (len && path[len - 1] == PATH_SEPARATOR) {
        path[len - 1] = '\0';
    }
    return path;
}