#pragma once
#include "config.h"

#define CONF_DIR ".moonlight-lite"
#define CONF_NAME_STREAMING "streaming.conf"

void config_save(char *filename, PCONFIGURATION config);

PCONFIGURATION settings_load();

void settings_save(PCONFIGURATION config);