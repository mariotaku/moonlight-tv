#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

static char *_path_join(const char *parent, const char *basename)
{
    char *joined = calloc(strlen(parent) + strlen(basename), sizeof(char));
    sprintf(joined, "%s/%s", parent, basename);
    return joined;
}

static char *settings_config_dir()
{
    char *homedir = getenv("HOME");
    char *confdir = _path_join(homedir, CONF_DIR);
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
    char *argv[2] = {"moonlight", _path_join(confdir, CONF_NAME_STREAMING)};
    config_parse(2, argv, config);
    free(argv[1]);

    config->unsupported = true;

    return config;
}