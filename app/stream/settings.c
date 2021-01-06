#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

static char *_path_join(const char *parent, const char *basename)
{
    char *joined = calloc(strlen(parent) + 1 + strlen(basename) + 1, sizeof(char));
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

static void settings_initialize(char *confdir, PCONFIGURATION config);

PCONFIGURATION settings_load()
{
    PCONFIGURATION config = malloc(sizeof(CONFIGURATION));
    char *confdir = settings_config_dir(), *conffile = _path_join(confdir, CONF_NAME_STREAMING);
    settings_initialize(confdir, config);
    config_file_parse(conffile, config);
    free(conffile);
    free(confdir);
    return config;
}

void settings_save(PCONFIGURATION config)
{
    char *confdir = settings_config_dir(), *conffile = _path_join(confdir, CONF_NAME_STREAMING);
    config_save(conffile, config);
    free(conffile);
    free(confdir);
}

void settings_initialize(char *confdir, PCONFIGURATION config)
{
    memset(config, 0, sizeof(CONFIGURATION));
    LiInitializeStreamConfiguration(&config->stream);

    config->stream.width = 1280;
    config->stream.height = 720;
    config->stream.fps = 60;
    config->stream.bitrate = -1;
    config->stream.packetSize = 1024;
    config->stream.streamingRemotely = 0;
    config->stream.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
    config->stream.supportsHevc = false;

    config->debug_level = 0;
    config->platform = "auto";
    config->app = "Steam";
    config->action = NULL;
    config->address = NULL;
    config->config_file = NULL;
    config->audio_device = NULL;
    config->sops = true;
    config->localaudio = false;
    config->fullscreen = true;
    // TODO make this automatic
    config->unsupported = true;
    config->quitappafter = false;
    config->viewonly = false;
    config->rotate = 0;
    config->codec = CODEC_UNSPECIFIED;

    config->inputsCount = 0;
    config->mapping = NULL;
    sprintf(config->key_dir, "%s/%s", confdir, "key");
}