#pragma once

#include "client.h"

int gs_conf_load(GS_CLIENT hnd, const char *keydir);

int gs_conf_init(const char *keydir);
