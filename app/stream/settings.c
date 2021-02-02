#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/stat.h>

#include "util/path.h"

static void settings_initialize(char *confdir, PCONFIGURATION config);
static bool settings_read(char *filename, PCONFIGURATION config);
static void settings_write(char *filename, PCONFIGURATION config);
static void parse_argument(int c, char* value, PCONFIGURATION config);

#define write_config_string(fd, key, value) fprintf(fd, "%s = %s\n", key, value)
#define write_config_int(fd, key, value) fprintf(fd, "%s = %d\n", key, value)
#define write_config_bool(fd, key, value) fprintf(fd, "%s = %s\n", key, value ? "true":"false")

static struct option long_options[] = {
  {"720", no_argument, NULL, 'a'},
  {"1080", no_argument, NULL, 'b'},
  {"4k", no_argument, NULL, '0'},
  {"width", required_argument, NULL, 'c'},
  {"height", required_argument, NULL, 'd'},
  {"bitrate", required_argument, NULL, 'g'},
  {"packetsize", required_argument, NULL, 'h'},
  {"app", required_argument, NULL, 'i'},
  {"input", required_argument, NULL, 'j'},
  {"mapping", required_argument, NULL, 'k'},
  {"nosops", no_argument, NULL, 'l'},
  {"audio", required_argument, NULL, 'm'},
  {"localaudio", no_argument, NULL, 'n'},
  {"config", required_argument, NULL, 'o'},
  {"platform", required_argument, NULL, 'p'},
  {"save", required_argument, NULL, 'q'},
  {"keydir", required_argument, NULL, 'r'},
  {"remote", no_argument, NULL, 's'},
  {"windowed", no_argument, NULL, 't'},
  {"surround", no_argument, NULL, 'u'},
  {"fps", required_argument, NULL, 'v'},
  {"codec", required_argument, NULL, 'x'},
  {"unsupported", no_argument, NULL, 'y'},
  {"quitappafter", no_argument, NULL, '1'},
  {"viewonly", no_argument, NULL, '2'},
  {"rotate", required_argument, NULL, '3'},
  {"verbose", no_argument, NULL, 'z'},
  {"debug", no_argument, NULL, 'Z'},
  {0, 0, 0, 0},
};

PCONFIGURATION settings_load()
{
    PCONFIGURATION config = malloc(sizeof(CONFIGURATION));
    char *confdir = settings_config_dir(), *conffile = path_join(confdir, CONF_NAME_STREAMING);
    settings_initialize(confdir, config);
    settings_read(conffile, config);
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
    config->stream.bitrate = settings_optimal_bitrate(1280, 720, 60);
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
    int kbps = w * h / 150;
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
        kbps = 25000;
        break;
    }
    return kbps * fps / 30;
}

bool settings_sops_supported(int w, int h, int fps)
{
    if (fps != 30 && fps != 60)
    {
        return false;
    }
    switch (RES_MERGE(w, h))
    {
    case RES_720P:
    case RES_1080P:
    case RES_4K:
        return true;
    default:
        return false;
    }
}

bool settings_read(char *filename, PCONFIGURATION config)
{
    FILE *fd = fopen(filename, "r");
    if (fd == NULL)
    {
        fprintf(stderr, "Can't open configuration file: %s\n", filename);
        return false;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fd) != -1)
    {
        char key[256], valtmp[256];
        if (sscanf(line, "%s = %s", key, valtmp) == 2)
        {
            char *value = strdup(valtmp);
            if (strcmp(key, "address") == 0)
            {
                config->address = value;
            }
            else if (strcmp(key, "sops") == 0)
            {
                config->sops = strcmp("true", value) == 0;
                free(value);
            }
            else
            {
                for (int i = 0; long_options[i].name != NULL; i++)
                {
                    if (strcmp(long_options[i].name, key) == 0)
                    {
                        if (long_options[i].has_arg == required_argument)
                            parse_argument(long_options[i].val, value, config);
                        else if (strcmp("true", value) == 0)
                            parse_argument(long_options[i].val, NULL, config);

                        switch (long_options[i].val)
                        {
                        case 'i':
                        case 'm':
                        case 'p':
                        case 'q':
                            break;
                        default:
                            free(value);
                            break;
                        }
                    }
                }
            }
        }
    }
    return true;
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
    if (strcmp(config->platform, "auto") != 0)
        write_config_string(fd, "platform", config->platform);
    if (config->audio_device != NULL)
        write_config_string(fd, "audio", config->audio_device);
    if (config->address != NULL)
        write_config_string(fd, "address", config->address);

    if (strcmp(config->app, "Steam") != 0)
        write_config_string(fd, "app", config->app);

    fclose(fd);
}

char *settings_config_dir()
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

 void parse_argument(int c, char* value, PCONFIGURATION config) {
  switch (c) {
  case 'a':
    config->stream.width = 1280;
    config->stream.height = 720;
    break;
  case 'b':
    config->stream.width = 1920;
    config->stream.height = 1080;
    break;
  case '0':
    config->stream.width = 3840;
    config->stream.height = 2160;
    break;
  case 'c':
    config->stream.width = atoi(value);
    break;
  case 'd':
    config->stream.height = atoi(value);
    break;
  case 'g':
    config->stream.bitrate = atoi(value);
    break;
  case 'h':
    config->stream.packetSize = atoi(value);
    break;
  case 'i':
    config->app = value;
    break;
  case 'l':
    config->sops = false;
    break;
  case 'm':
    config->audio_device = value;
    break;
  case 'n':
    config->localaudio = true;
    break;
  case 'p':
    config->platform = value;
    break;
  case 'q':
    config->config_file = value;
    break;
  case 'r':
    strcpy(config->key_dir, value);
    break;
  case 's':
    config->stream.streamingRemotely = 1;
    break;
  case 't':
    config->fullscreen = false;
    break;
  case 'u':
    config->stream.audioConfiguration = AUDIO_CONFIGURATION_51_SURROUND;
    break;
  case 'v':
    config->stream.fps = atoi(value);
    break;
  case 'x':
    if (strcasecmp(value, "auto") == 0)
      config->codec = CODEC_UNSPECIFIED;
    else if (strcasecmp(value, "h264") == 0)
      config->codec = CODEC_H264;
    if (strcasecmp(value, "h265") == 0 || strcasecmp(value, "hevc") == 0)
      config->codec = CODEC_HEVC;
    break;
  case 'y':
    config->unsupported = true;
    break;
  case '1':
    config->quitappafter = true;
    break;
  case '2':
    config->viewonly = true;
    break;
  case '3':
    config->rotate = atoi(value);
    break;
  case 'z':
    config->debug_level = 1;
    break;
  case 'Z':
    config->debug_level = 2;
    break;
  case 1:
    if (config->action == NULL)
      config->action = value;
    else if (config->address == NULL)
      config->address = value;
    else {
      perror("Too many options");
      exit(-1);
    }
  }
}