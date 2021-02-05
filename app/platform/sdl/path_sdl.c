#include "util/path.h"

#include <SDL.h>

char *path_pref()
{
    return SDL_GetPrefPath("com.limelight", "moonlight-tv");
}