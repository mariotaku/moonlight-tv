#include "settings.h"

#include <stdlib.h>
#include <string.h>

#include "ini.h"

#include "stream/platform.h"

#include "util/ini_ext.h"
#include "util/nullable.h"
#include "util/path.h"
#include "util/logging.h"

static void settings_initialize(const char *confdir, PCONFIGURATION config);

static bool settings_read(const char *filename, PCONFIGURATION config);

static void settings_write(const char *filename, PCONFIGURATION config);

static int find_ch_idx_by_config(int config);

static int find_ch_idx_by_value(const char *value);

static const char *serialize_audio_config(int config);

static int parse_audio_config(const char *value);

static int settings_parse(PCONFIGURATION config, const char *section, const char *name, const char *value);

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
    free_nullable(config->language);
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
    config->stream.supportsHevc = true;

    config->debug_level = 0;
    config->language = strdup("auto");
    config->audio_backend = strdup("auto");
    config->decoder = strdup("auto");
    config->audio_device = NULL;
    config->sops = true;
    config->localaudio = false;
    config->fullscreen = true;
    // TODO make this automatic
    config->unsupported = true;
    config->quitappafter = false;
    config->viewonly = false;
    config->rotate = 0;
    config->absmouse = true;
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
    int suggested_max = decoder_info.suggestedBitrate;
    if (!suggested_max) {
        suggested_max = decoder_info.maxFramerate;
    }
    int calculated = kbps * fps / 30;
    if (!suggested_max) {
        return calculated;
    }
    return calculated < suggested_max ? calculated : suggested_max;
}

bool settings_read(const char *filename, PCONFIGURATION config) {
    return ini_parse(filename, (ini_handler) settings_parse, config) == 0;
}

void settings_write(const char *filename, PCONFIGURATION config) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return;
    ini_write_string(fp, "language", config->language);
    ini_write_bool(fp, "fullscreen", config->fullscreen);
    ini_write_int(fp, "debug_level", config->debug_level);

    ini_write_section(fp, "streaming");
    ini_write_int(fp, "width", config->stream.width);
    ini_write_int(fp, "height", config->stream.height);
    ini_write_int(fp, "net_fps", config->stream.fps);
    ini_write_int(fp, "bitrate", config->stream.bitrate);
    ini_write_int(fp, "packetsize", config->stream.packetSize);
    ini_write_int(fp, "rotate", config->rotate);

    ini_write_section(fp, "host");
    ini_write_bool(fp, "sops", config->sops);
    ini_write_bool(fp, "localaudio", config->localaudio);
    ini_write_bool(fp, "quitappafter", config->quitappafter);
    ini_write_bool(fp, "viewonly", config->viewonly);

    ini_write_section(fp, "input");
    ini_write_bool(fp, "absmouse", config->absmouse);
    ini_write_bool(fp, "swap_abxy", config->swap_abxy);

    ini_write_section(fp, "video");
    ini_write_string(fp, "decoder", config->decoder);
    ini_write_bool(fp, "hdr", config->stream.enableHdr);
    ini_write_bool(fp, "hevc", config->stream.supportsHevc);

    ini_write_section(fp, "audio");
    ini_write_string(fp, "backend", config->audio_backend);
    if (config->audio_device && config->audio_device[0]) {
        ini_write_string(fp, "device", config->audio_device);
    }
    ini_write_string(fp, "surround", serialize_audio_config(config->stream.audioConfiguration));

    fclose(fp);
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

static int settings_parse(PCONFIGURATION config, const char *section, const char *name, const char *value) {
    if (INI_FULL_MATCH("streaming", "width")) {
        config->stream.width = atoi(value);
    } else if (INI_FULL_MATCH("streaming", "height")) {
        config->stream.height = atoi(value);
    } else if (INI_FULL_MATCH("streaming", "net_fps")) {
        config->stream.fps = atoi(value);
    } else if (INI_FULL_MATCH("streaming", "bitrate")) {
        config->stream.bitrate = atoi(value);
    } else if (INI_FULL_MATCH("streaming", "packetsize")) {
        config->stream.packetSize = atoi(value);
    } else if (INI_FULL_MATCH("streaming", "rotate")) {
        config->rotate = atoi(value);
    } else if (INI_NAME_MATCH("hevc")) {
        config->stream.supportsHevc = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("hdr")) {
        config->stream.enableHdr = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("surround")) {
        config->stream.audioConfiguration = parse_audio_config(value);
    } else if (INI_NAME_MATCH("sops")) {
        config->sops = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("localaudio")) {
        config->localaudio = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("quitappafter")) {
        config->quitappafter = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("viewonly")) {
        config->viewonly = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("absmouse")) {
        config->absmouse = INI_IS_TRUE(value);
    } else if (INI_NAME_MATCH("swap_abxy")) {
        config->swap_abxy = INI_IS_TRUE(value);
    } else if (INI_FULL_MATCH("video", "decoder")) {
        config->decoder = strdup(value);
    } else if (INI_FULL_MATCH("audio", "backend")) {
        config->audio_backend = strdup(value);
    } else if (INI_FULL_MATCH("audio", "device")) {
        config->audio_device = strdup(value);
    } else if (INI_NAME_MATCH("language")) {
        config->language = strdup(value);
    } else if (INI_NAME_MATCH("fullscreen")) {
#if TARGET_WEBOS
        config->fullscreen = true;
#else
        config->fullscreen = INI_IS_TRUE(value);
#endif
    } else if (INI_NAME_MATCH("debug_level")) {
        config->debug_level = atoi(value);
    }
    return 1;
}