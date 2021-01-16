#include "src/config.c"
#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "util/path.h"

static char *settings_config_dir()
{
    char *homedir = getenv("HOME");
    char *confdir = path_join(homedir, CONF_DIR);
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
static void settings_write(char *filename, PCONFIGURATION config);

PCONFIGURATION settings_load()
{
    PCONFIGURATION config = malloc(sizeof(CONFIGURATION));
    char *confdir = settings_config_dir(), *conffile = path_join(confdir, CONF_NAME_STREAMING);
    settings_initialize(confdir, config);
    config_file_parse(conffile, config);
    free(conffile);
    free(confdir);
    return config;
}

void settings_save(PCONFIGURATION config)
{
    char *confdir = settings_config_dir(), *conffile = path_join(confdir, CONF_NAME_STREAMING);
    settings_write(conffile, config);
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

    config->debug_level = 1;
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

int settings_optimal_bitrate(int w, int h, int fps)
{
    if (fps <= 0)
    {
        fps = 60;
    }
    int kbps = w * h / 200;
    switch (RES_MERGE(w, h))
    {
    case RES_720P:
        kbps = 5000;
        break;
    case RES_1080P:
        kbps = 10000;
        break;
    case RES_2K:
        kbps = 20000;
        break;
    case RES_4K:
        kbps = 40000;
        break;
    }
    return kbps * fps / 30;
}

void settings_write(char *filename, PCONFIGURATION config)
{
    FILE *fd = fopen(filename, "w");
    if (fd == NULL)
    {
        fprintf(stderr, "Can't open configuration file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    if (config->stream.width != 1280)
        write_config_int(fd, "width", config->stream.width);
    if (config->stream.height != 720)
        write_config_int(fd, "height", config->stream.height);
    if (config->stream.fps != 60)
        write_config_int(fd, "fps", config->stream.fps);
    if (config->stream.bitrate != -1)
        write_config_int(fd, "bitrate", config->stream.bitrate);
    if (config->stream.packetSize != 1024)
        write_config_int(fd, "packetsize", config->stream.packetSize);
    if (!config->sops)
        write_config_bool(fd, "sops", config->sops);
    if (config->localaudio)
        write_config_bool(fd, "localaudio", config->localaudio);
    if (config->quitappafter)
        write_config_bool(fd, "quitappafter", config->quitappafter);
    if (config->viewonly)
        write_config_bool(fd, "viewonly", config->viewonly);
    if (config->rotate != 0)
        write_config_int(fd, "rotate", config->rotate);
    if (config->platform != NULL)
        write_config_string(fd, "platform", config->platform);

    if (strcmp(config->app, "Steam") != 0)
        write_config_string(fd, "app", config->app);

    fclose(fd);
}
