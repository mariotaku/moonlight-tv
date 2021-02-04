#include "util/path.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL2/SDL.h>

char *path_pref()
{
    char *basedir = SDL_GetPrefPath("com.limelight", "moonlight-tv");
    char *confdir = path_join(basedir, "conf");
    if (access(confdir, F_OK) == -1)
    {
        if (errno == ENOENT)
        {
            mkdir(confdir, 0755);
        }
    }
    free(basedir);
    return confdir;
}