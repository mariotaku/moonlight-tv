#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/stat.h>

#include "util/path.h"
#include "util/logging.h"
#include "util/memlog.h"

static void settings_initialize(char *confdir, PCONFIGURATION config);
static bool settings_read(char *filename, PCONFIGURATION config);
static void settings_write(char *filename, PCONFIGURATION config);
static void parse_argument(int c, char *value, PCONFIGURATION config);

static void write_config_absmouse_mapping(FILE *fd, char *key, ABSMOUSE_MAPPING mapping);
static bool parse_config_absmouse_mapping(char *value, ABSMOUSE_MAPPING *mapping);

static int find_ch_idx_by_config(int config);
static int find_ch_idx_by_value(const char *value);
static void write_audio_config(FILE *fd, char *key, int config);
static bool parse_audio_config(const char *value, int *config);

#define write_config_string(fd, key, value) fprintf(fd, "%s = %s\n", key, value)
#define write_config_int(fd, key, value) fprintf(fd, "%s = %d\n", key, value)
#define write_config_bool(fd, key, value) fprintf(fd, "%s = %s\n", key, value ? "true" : "false")

#define SHORT_OPTION_SURROUND 'u'

static struct option long_options[] = {
    {"width", required_argument, NULL, 'c'},
    {"height", required_argument, NULL, 'd'},
    {"absmouse_mapping", required_argument, NULL, 'e'},
    {"bitrate", required_argument, NULL, 'g'},
    {"hdr", no_argument, NULL, 'h'},
    {"packetsize", required_argument, NULL, 'i'},
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
    {"surround", required_argument, NULL, SHORT_OPTION_SURROUND},
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
struct audio_config
{
    int configuration;
    const char *value;
};

static struct audio_config audio_configs[3] = {
    {AUDIO_CONFIGURATION_STEREO, "stereo"},
    {AUDIO_CONFIGURATION_51_SURROUND, "5.1ch"},
    {AUDIO_CONFIGURATION_71_SURROUND, "7.1ch"},
};
static const int audio_config_len = sizeof(audio_configs) / sizeof(struct audio_config);

PCONFIGURATION settings_load()
{
    PCONFIGURATION config = malloc(sizeof(CONFIGURATION));
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_STREAMING);
    settings_initialize(confdir, config);
    settings_read(conffile, config);
    free(conffile);
    free(confdir);
    return config;
}

void settings_save(PCONFIGURATION config)
{
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_STREAMING);
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

    config->debug_level = 0;
    config->platform = "auto";
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
    case RES_1440P:
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
        applog_i("Settings", "Can't open configuration file: %s", filename);
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
        applog_e("Settings", "Can't open configuration file for writing: %s", filename);
        return;
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
    if (audio_config_valid(config->stream.audioConfiguration))
        write_audio_config(fd, "surround", config->stream.audioConfiguration);
    if (config->address != NULL)
        write_config_string(fd, "address", config->address);
    if (config->stream.enableHdr)
        write_config_bool(fd, "hdr", true);
    if (config->debug_level == 1)
        write_config_bool(fd, "verbose", true);
    else if (config->debug_level == 2)
        write_config_bool(fd, "debug", true);
    if (absmouse_mapping_valid(config->absmouse_mapping))
        write_config_absmouse_mapping(fd, "absmouse_mapping", config->absmouse_mapping);

    fclose(fd);
}

void parse_argument(int c, char *value, PCONFIGURATION config)
{
    bool free_value = true;
    switch (c)
    {
    case 'c':
        config->stream.width = atoi(value);
        break;
    case 'd':
        config->stream.height = atoi(value);
        break;
    case 'e':
        parse_config_absmouse_mapping(value, &config->absmouse_mapping);
        break;
    case 'g':
        config->stream.bitrate = atoi(value);
        break;
    case 'h':
        config->stream.enableHdr = true;
        break;
    case 'i':
        config->stream.packetSize = atoi(value);
        break;
    case 'l':
        config->sops = false;
        break;
    case 'm':
        config->audio_device = value;
        free_value = false;
        break;
    case 'n':
        config->localaudio = true;
        break;
    case 'p':
        config->platform = value;
        free_value = false;
        break;
    case 'q':
        config->config_file = value;
        free_value = false;
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
    case SHORT_OPTION_SURROUND:
        parse_audio_config(value, &config->stream.audioConfiguration);
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
        if (config->address == NULL)
        {
            config->address = value;
            free_value = false;
        }
    }
    if (free_value && value)
    {
        free(value);
    }
}

bool absmouse_mapping_valid(ABSMOUSE_MAPPING mapping)
{
    return mapping.desktop_w && mapping.desktop_h && mapping.screen_w && mapping.screen_h;
}

bool audio_config_valid(int config)
{
    return find_ch_idx_by_config(config) >= 0;
}

void write_config_absmouse_mapping(FILE *fd, char *key, ABSMOUSE_MAPPING mapping)
{
    fprintf(fd, "%s = [%d,%d][%d*%d]@[%d*%d]\n", key, mapping.screen_x, mapping.screen_y, mapping.screen_w, mapping.screen_h,
            mapping.desktop_w, mapping.desktop_h);
}

bool parse_config_absmouse_mapping(char *value, ABSMOUSE_MAPPING *mapping)
{
    int screen_x, screen_y, screen_w, screen_h, desktop_w, desktop_h;
    if (sscanf(value, "[%d,%d][%d*%d]@[%d*%d]", &screen_x, &screen_y, &screen_w, &screen_h, &desktop_w, &desktop_h) != 6)
    {
        return false;
    }
    mapping->desktop_w = desktop_w;
    mapping->desktop_h = desktop_h;
    mapping->screen_w = screen_w;
    mapping->screen_h = screen_h;
    mapping->screen_x = screen_x;
    mapping->screen_y = screen_y;
    return true;
}

void write_audio_config(FILE *fd, char *key, int config)
{
    fprintf(fd, "%s = %s\n", key, audio_configs[find_ch_idx_by_config(config)].value);
}

bool parse_audio_config(const char *value, int *config)
{
    int index = find_ch_idx_by_value(value);
    if (index < 0)
        return false;
    *config = audio_configs[index].configuration;
    return true;
}

int find_ch_idx_by_config(int config)
{
    for (int i = 0; i < audio_config_len; i++)
    {
        if (audio_configs[i].configuration == config)
            return i;
    }
    return -1;
}

int find_ch_idx_by_value(const char *value)
{
    for (int i = 0; i < audio_config_len; i++)
    {
        if (strcmp(audio_configs[i].value, value) == 0)
            return i;
    }
    return -1;
}