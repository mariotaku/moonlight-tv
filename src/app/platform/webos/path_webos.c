#include "util/path.h"

#include <SDL.h>

char *path_assets() {
    const char *basedir = SDL_getenv("HOME");
    return path_join(basedir, "assets");
}

char *path_pref() {
    const char *basedir = SDL_getenv("HOME");
    char *confdir = path_join(basedir, "conf");
    path_dir_ensure(confdir);
    return confdir;
}

char *path_cache() {
    const char *basedir = SDL_getenv("HOME");
    char *cachedir = path_join(basedir, "cache");
    if (path_dir_ensure(cachedir) == -1) {
        free(cachedir);
        cachedir = strdup("/tmp/moonlight-tv-cache");
        path_dir_ensure(cachedir);
    }
    return cachedir;
}