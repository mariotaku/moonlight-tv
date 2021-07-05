#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libconfig.h>

#include <sys/stat.h>

#include "util/path.h"
#include "util/logging.h"
#include "util/memlog.h"

static void settings_initialize(char *confdir, PCONFIGURATION config);
static bool settings_read(char *filename, PCONFIGURATION config);
static void settings_write(char *filename, PCONFIGURATION config);

static int find_ch_idx_by_config(int config);
static int find_ch_idx_by_value(const char *value);
static const char *serialize_audio_config(int config);
static int parse_audio_config(const char *value);

static inline int config_setting_set_enum(config_setting_t *setting, int value, const char *(*converter)(int))
{
    return config_setting_set_string(setting, converter(value));
}

static inline int config_lookup_enum(const config_t *config, const char *path, int *value, int (*converter)(const char *))
{
    int ret;
    const char *str = NULL;
    if ((ret = config_lookup_string(config, path, &str)) == CONFIG_TRUE)
    {
        *value = converter(str);
    }
    return ret;
}

static inline int config_lookup_string_dup(const config_t *config, const char *path, const char **value)
{
    int ret;
    if ((ret = config_lookup_string(config, path, value)) == CONFIG_TRUE && *value)
    {
        *value = strdup(*value);
    }
    return ret;
}

static inline void write_config_string(config_setting_t *parent, const char *key, const char *value)
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_STRING);
    config_setting_set_string(setting, value);
}
static inline void write_config_int(config_setting_t *parent, const char *key, int value)
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_INT);
    config_setting_set_int(setting, value);
}
static inline void write_config_bool(config_setting_t *parent, const char *key, bool value)
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_BOOL);
    config_setting_set_bool(setting, value);
}

static inline int write_config_enum(config_setting_t *parent, const char *key, int value, const char *(*converter)(int))
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_STRING);
    return config_setting_set_enum(setting, value, converter);
}

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
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_MOONLIGHT);
    settings_initialize(confdir, config);
    settings_read(conffile, config);
    free(conffile);
    free(confdir);
    return config;
}

void settings_save(PCONFIGURATION config)
{
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_MOONLIGHT);
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
    config->audio_backend = "auto";
    config->decoder = "auto";
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

bool settings_read(char *filename, PCONFIGURATION config)
{
    struct config_t libconfig;
    config_init(&libconfig);
    int options = config_get_options(&libconfig);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&libconfig, options);

    if (config_read_file(&libconfig, filename) != CONFIG_TRUE)
    {
        config_destroy(&libconfig);
        applog_i("Settings", "Can't open configuration file: %s", filename);
        return false;
    }

    config_lookup_int(&libconfig, "streaming.width", &config->stream.width);
    config_lookup_int(&libconfig, "streaming.height", &config->stream.height);
    config_lookup_int(&libconfig, "streaming.fps", &config->stream.fps);
    config_lookup_int(&libconfig, "streaming.bitrate", &config->stream.bitrate);
    config_lookup_int(&libconfig, "streaming.packetsize", &config->stream.packetSize);
    config_lookup_int(&libconfig, "streaming.rotate", &config->rotate);
    config_lookup_bool(&libconfig, "streaming.hdr", (int *)&config->stream.enableHdr);
    config_lookup_enum(&libconfig, "streaming.surround", &config->stream.audioConfiguration, parse_audio_config);

    config_lookup_bool(&libconfig, "host.sops", (int *)&config->sops);
    config_lookup_bool(&libconfig, "host.localaudio", (int *)&config->localaudio);
    config_lookup_bool(&libconfig, "host.quitappafter", (int *)&config->quitappafter);
    config_lookup_bool(&libconfig, "host.viewonly", (int *)&config->viewonly);

    config_lookup_string_dup(&libconfig, "decoder.platform", &config->decoder);
    config_lookup_string_dup(&libconfig, "decoder.audio_backend", &config->audio_backend);
    config_lookup_string_dup(&libconfig, "decoder.audio_device", &config->audio_device);

    config_lookup_int(&libconfig, "debug_level", &config->debug_level);

    config_destroy(&libconfig);
    return true;
}

void settings_write(char *filename, PCONFIGURATION config)
{
    struct config_t libconfig;
    config_init(&libconfig);
    int options = config_get_options(&libconfig);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&libconfig, options);

    config_setting_t *root = config_root_setting(&libconfig);
    config_setting_t *streaming = config_setting_add(root, "streaming", CONFIG_TYPE_GROUP);
    config_setting_t *host = config_setting_add(root, "host", CONFIG_TYPE_GROUP);
    config_setting_t *decoder = config_setting_add(root, "decoder", CONFIG_TYPE_GROUP);

    write_config_int(streaming, "width", config->stream.width);
    write_config_int(streaming, "height", config->stream.height);
    write_config_int(streaming, "fps", config->stream.fps);
    write_config_int(streaming, "bitrate", config->stream.bitrate);
    write_config_int(streaming, "packetsize", config->stream.packetSize);
    write_config_bool(host, "sops", config->sops);
    write_config_bool(host, "localaudio", config->localaudio);
    write_config_bool(host, "quitappafter", config->quitappafter);
    write_config_bool(host, "viewonly", config->viewonly);
    write_config_int(streaming, "rotate", config->rotate);
    write_config_string(decoder, "platform", config->decoder);
    write_config_string(decoder, "audio_backend", config->audio_backend);
    if (!config->audio_device || !config->audio_device[0])
        config_setting_remove(decoder, "audio_device");
    else
        write_config_string(decoder, "audio_device", config->audio_device);
    write_config_enum(streaming, "surround", config->stream.audioConfiguration, serialize_audio_config);

    write_config_bool(streaming, "hdr", config->stream.enableHdr);
    write_config_int(root, "debug_level", config->debug_level);

    if (config_write_file(&libconfig, filename) != CONFIG_TRUE)
    {
        applog_e("Settings", "Can't open configuration file for writing: %s", filename);
    }
    config_destroy(&libconfig);
}

bool audio_config_valid(int config)
{
    return find_ch_idx_by_config(config) >= 0;
}

const char *serialize_audio_config(int config)
{
    return audio_configs[find_ch_idx_by_config(config)].value;
}

int parse_audio_config(const char *value)
{
    int index = value ? find_ch_idx_by_value(value) : -1;
    if (index < 0)
        index = 0;
    return audio_configs[index].configuration;
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