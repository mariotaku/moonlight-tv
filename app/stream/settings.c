#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

static const char conf_dir[] = ".moonlight-sdl";
static const char conf_name_streaming[] = "streaming.conf";

static char *_path_join(const char *parent, const char *basename)
{
    char *joined = calloc(strlen(parent) + strlen(basename), sizeof(char));
    sprintf(joined, "%s/%s", parent, basename);
    return joined;
}

static char *settings_config_dir()
{
    char *homedir = getenv("HOME");
    char *confdir = _path_join(homedir, conf_dir);
    if (access(confdir, F_OK) == -1)
    {
        if (errno == ENOENT)
        {
            mkdir(confdir, 0755);
        }
    }
    return confdir;
}

PCONFIGURATION settings_load()
{
    PCONFIGURATION config = malloc(sizeof(CONFIGURATION));
    char *confdir = settings_config_dir();
    char *argv[2] = {"moonlight", _path_join(confdir, conf_name_streaming)};
    config_parse(2, argv, config);
    free(argv[1]);

    config->unsupported = true;

    return config;
}