#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libconfig.h>

#include <sys/stat.h>
#include <util/nullable.h>

#include "util/path.h"
#include "util/logging.h"
#include "util/libconfig_ext.h"

#if TARGET_WEBOS
#define DEFAULT_ABSMOUSE true
#else
#define DEFAULT_ABSMOUSE false
#endif

static void settings_initialize(const char *confdir, PCONFIGURATION config);

static bool settings_read(char *filename, PCONFIGURATION config);

static void settings_write(char *filename, PCONFIGURATION config);

static int find_ch_idx_by_config(int config);

static int find_ch_idx_by_value(const char *value);

static const char *serialize_audio_config(int config);

static int parse_audio_config(const char *value);

struct audio_config {
    int configuration;
    const char *value;
};

static struct audio_config audio_configs[3] = {
        {AUDIO_CONFIGURATION_STEREO,      "stereo"},
        {AUDIO_CONFIGURATION_51_SURROUND, "5.1ch"},
        {AUDIO_CONFIGURATION_71_SURROUND, "7.1ch"},
};
static const int audio_config_len = sizeof(audio_configs) / sizeof(struct audio_config);

PCONFIGURATION settings_load() {
    PCONFIGURATION config = malloc(sizeof(CONFIGURATION));
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_MOONLIGHT);
    settings_initialize(confdir, config);
    settings_read(conffile, config);
    free(conffile);
    free(confdir);
    return config;
}

void settings_save(PCONFIGURATION config) {
    char *confdir = path_pref(), *conffile = path_join(confdir, CONF_NAME_MOONLIGHT);
    settings_write(conffile, config);
    free(conffile);
    free(confdir);
}

void settings_free(PCONFIGURATION config) {
    free_nullable(config->decoder);
    free_nullable(config->audio_backend);
    free_nullable(config->audio_device);
    free(config);
}

void settings_initialize(const char *confdir, PCONFIGURATION config) {
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
    config->audio_backend = strdup("auto");
    config->decoder = strdup("auto");
    config->audio_device = NULL;
    config->sops = true;
    config->localaudio = false;
#if TARGET_RASPI
    config->fullscreen = true;
#endif
    // TODO make this automatic
    config->unsupported = true;
    config->quitappafter = false;
    config->viewonly = false;
    config->rotate = 0;
    config->codec = CODEC_UNSPECIFIED;
    config->absmouse = DEFAULT_ABSMOUSE;
    path_join_to(config->key_dir, sizeof(config->key_dir), confdir, "key");
}

int settings_optimal_bitrate(int w, int h, int fps) {
    if (fps <= 0) {
        fps = 60;
    }
    int kbps = w * h / 150;
    switch (RES_MERGE(w, h)) {
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

bool settings_read(char *filename, PCONFIGURATION config) {
    struct config_t libconfig;
    config_init(&libconfig);
    int options = config_get_options(&libconfig);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&libconfig, options);

    if (config_read_file(&libconfig, filename) != CONFIG_TRUE) {
        config_destroy(&libconfig);
        applog_i("Settings", "Can't open configuration file: %s", filename);
        return false;
    }

    config_lookup_int(&libconfig, "streaming.width", &config->stream.width);
    config_lookup_int(&libconfig, "streaming.height", &config->stream.height);
    config_lookup_int(&libconfig, "streaming.net_fps", &config->stream.fps);
    config_lookup_int(&libconfig, "streaming.bitrate", &config->stream.bitrate);
    config_lookup_int(&libconfig, "streaming.packetsize", &config->stream.packetSize);
    config_lookup_int(&libconfig, "streaming.rotate", &config->rotate);
    config_lookup_bool_std(&libconfig, "streaming.hdr", &config->stream.enableHdr);
    config_lookup_enum(&libconfig, "streaming.surround", &config->stream.audioConfiguration, parse_audio_config);

    config_lookup_bool_std(&libconfig, "host.sops", &config->sops);
    config_lookup_bool_std(&libconfig, "host.localaudio", &config->localaudio);
    config_lookup_bool_std(&libconfig, "host.quitappafter", &config->quitappafter);

    config_lookup_bool_std(&libconfig, "host.viewonly", &config->viewonly);
    config_lookup_bool_std(&libconfig, "input.absmouse", &config->absmouse);
    config_lookup_bool_std(&libconfig, "input.swap_abxy", &config->swap_abxy);

    char *str_tmp = NULL;
    if (config_lookup_string_dup(&libconfig, "decoder.platform", &str_tmp)) {
        free_nullable(config->decoder);
        config->decoder = str_tmp;
    }
    if (config_lookup_string_dup(&libconfig, "decoder.audio_backend", &str_tmp)) {
        free_nullable(config->audio_backend);
        config->audio_backend = str_tmp;
    }
    if (config_lookup_string_dup(&libconfig, "decoder.audio_device", &str_tmp)) {
        free_nullable(config->audio_device);
        config->audio_device = str_tmp;
    }

#if TARGET_WEBOS
    config->fullscreen = true;
#else
    config_lookup_bool_std(&libconfig, "fullscreen", &config->fullscreen);
#endif
    config_lookup_int(&libconfig, "debug_level", &config->debug_level);
    config->analytics_defined = config_lookup_int(&libconfig, "analytics", &config->analytics);

    config_destroy(&libconfig);
    return true;
}

void settings_write(char *filename, PCONFIGURATION config) {
    struct config_t libconfig;
    config_init(&libconfig);
    int options = config_get_options(&libconfig);
    options &= ~CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE;
    options &= ~CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS;
    config_set_options(&libconfig, options);

    config_setting_t *root = config_root_setting(&libconfig);
    config_setting_t *streaming = config_setting_add(root, "streaming", CONFIG_TYPE_GROUP);
    config_setting_t *host = config_setting_add(root, "host", CONFIG_TYPE_GROUP);
    config_setting_t *input = config_setting_add(root, "input", CONFIG_TYPE_GROUP);
    config_setting_t *decoder = config_setting_add(root, "decoder", CONFIG_TYPE_GROUP);

    config_setting_set_int_simple(streaming, "width", config->stream.width);
    config_setting_set_int_simple(streaming, "height", config->stream.height);
    config_setting_set_int_simple(streaming, "net_fps", config->stream.fps);
    config_setting_set_int_simple(streaming, "bitrate", config->stream.bitrate);
    config_setting_set_int_simple(streaming, "packetsize", config->stream.packetSize);
    config_setting_set_bool_simple(host, "sops", config->sops);
    config_setting_set_bool_simple(host, "localaudio", config->localaudio);
    config_setting_set_bool_simple(host, "quitappafter", config->quitappafter);
    config_setting_set_bool_simple(host, "viewonly", config->viewonly);
    config_setting_set_bool_simple(input, "absmouse", config->absmouse);
    config_setting_set_bool_simple(input, "swap_abxy", config->swap_abxy);
    config_setting_set_int_simple(streaming, "rotate", config->rotate);
    config_setting_set_string_simple(decoder, "platform", config->decoder);
    config_setting_set_string_simple(decoder, "audio_backend", config->audio_backend);
    if (!config->audio_device || !config->audio_device[0])
        config_setting_remove(decoder, "audio_device");
    else
        config_setting_set_string_simple(decoder, "audio_device", config->audio_device);
    config_setting_set_enum_simple(streaming, "surround", config->stream.audioConfiguration, serialize_audio_config);

    config_setting_set_bool_simple(streaming, "hdr", config->stream.enableHdr);
    config_setting_set_bool_simple(root, "fullscreen", config->fullscreen);
    config_setting_set_int_simple(root, "debug_level", config->debug_level);
    if (config->analytics_defined) {
        config_setting_set_int_simple(root, "analytics", config->analytics);
    }

    if (config_write_file(&libconfig, filename) != CONFIG_TRUE) {
        applog_e("Settings", "Can't open configuration file for writing: %s", filename);
    }
    config_destroy(&libconfig);
}

bool audio_config_valid(int config) {
    return find_ch_idx_by_config(config) >= 0;
}

const char *serialize_audio_config(int config) {
    return audio_configs[find_ch_idx_by_config(config)].value;
}

int parse_audio_config(const char *value) {
    int index = value ? find_ch_idx_by_value(value) : -1;
    if (index < 0)
        index = 0;
    return audio_configs[index].configuration;
}

int find_ch_idx_by_config(int config) {
    for (int i = 0; i < audio_config_len; i++) {
        if (audio_configs[i].configuration == config)
            return i;
    }
    return -1;
}

int find_ch_idx_by_value(const char *value) {
    for (int i = 0; i < audio_config_len; i++) {
        if (strcmp(audio_configs[i].value, value) == 0)
            return i;
    }
    return -1;
}