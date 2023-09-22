#include "util/path.h"

#include <SDL.h>

static char *path_cache_parent();

char *path_assets() {
    return SDL_GetBasePath();
}

char *path_pref(bool *persistent) {
    char *path = SDL_GetPrefPath("com.limelight", "moonlight-tv");
    unsigned int len = SDL_strlen(path);
    if (len && path[len - 1] == PATH_SEPARATOR) {
        path[len - 1] = '\0';
    }
    *persistent = true;
    return path;
}

char *path_cache() {
    char *cachedir = path_cache_parent();
    char *appcache = path_join(cachedir, "moonlight-tv");
    SDL_free(cachedir);
    path_dir_ensure(appcache);
    return appcache;
}

static char *path_cache_parent() {
#ifdef __WIN32
    char *appdata = SDL_getenv("LOCALAPPDATA");
    SDL_assert_release(appdata);
    char *appdir = path_join(appdata, "moonlight-tv");
    path_dir_ensure(appdir);
    char *cachedir = path_join(appdir, "cache");
    SDL_free(appdir);
    path_dir_ensure(cachedir);
    return cachedir;
#else
    char *cachedir = SDL_getenv("XDG_CACHE_DIR");
    if (cachedir) return SDL_strdup(cachedir);
    char *homedir = SDL_getenv("HOME");
    if (homedir) {
        cachedir = path_join(homedir, ".cache");
        path_dir_ensure(cachedir);
        return cachedir;
    }
    cachedir = path_join("/tmp", "moonlight-tv-cache");
    path_dir_ensure(cachedir);
    return cachedir;
#endif
}