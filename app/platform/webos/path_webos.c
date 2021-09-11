#include "util/path.h"

#include <SDL.h>

char *path_pref() {
    char *basedir = SDL_GetPrefPath("com.limelight", "moonlight-tv");
    char *confdir = path_join(basedir, "conf");
    SDL_free(basedir);
    path_dir_ensure(confdir);
    return confdir;
}

char *path_cache() {
    char *basedir = SDL_GetPrefPath("com.limelight", "moonlight-tv");
    char *cachedir = path_join(basedir, "cache");
    SDL_free(basedir);
    path_dir_ensure(cachedir);
    return cachedir;
}