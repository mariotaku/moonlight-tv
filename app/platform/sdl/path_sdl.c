#include "util/path.h"

#include <SDL.h>

static const char kPathSeparator =
#if defined _WIN32 || defined __CYGWIN__
    '\\';
#else
    '/';
#endif

char *path_pref()
{
    char *path = SDL_GetPrefPath("com.limelight", "moonlight-tv");
    int len = strlen(path);
    if (path[len - 1] == kPathSeparator)
    {
        path[len - 1] = '\0';
    }
    return path;
}