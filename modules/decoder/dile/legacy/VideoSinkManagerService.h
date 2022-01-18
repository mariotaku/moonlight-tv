#pragma once
#include "module/api.h"

#include <stdbool.h>

#define VideoSinkManagerRegisterVDEC PLUGIN_SYMBOL_NAME(VideoSinkManagerRegisterVDEC)
#define VideoSinkManagerRegisterPCMMC PLUGIN_SYMBOL_NAME(VideoSinkManagerRegisterPCMMC)
#define VideoSinkManagerUnregister PLUGIN_SYMBOL_NAME(VideoSinkManagerUnregister)

bool VideoSinkManagerRegisterVDEC(const char *contextId, int port);

bool VideoSinkManagerRegisterPCMMC(const char *contextId, int port, const char *audioType);

bool VideoSinkManagerUnregister(const char *contextId);