#include "util/path.h"

#include <SDL2/SDL.h>

char *path_pref()
{
    return SDL_GetPrefPath("com.limelight", "moonlight-tv");
}