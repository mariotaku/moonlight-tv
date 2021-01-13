#pragma once
#include "src/config.h"

#define CONF_DIR ".moonlight-tv"
#define CONF_NAME_STREAMING "streaming.conf"

#define RES_MERGE(w, h) ((w & 0xFFFF) << 16 | h & 0xFFFF)

#define RES_720P RES_MERGE(1280, 720)
#define RES_1080P RES_MERGE(1920, 1080)
#define RES_2K RES_MERGE(2560, 1440)
#define RES_4K RES_MERGE(3840, 2160)

void config_save(char *filename, PCONFIGURATION config);

PCONFIGURATION settings_load();

void settings_save(PCONFIGURATION config);

int settings_optimal_bitrate(int w, int h, int fps);