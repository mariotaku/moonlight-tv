#pragma once
#include "stream/api.h"

#include <stdbool.h>

#define VideoSinkManagerRegisterVDEC DECODER_SYMBOL_NAME(VideoSinkManagerRegisterVDEC)
#define VideoSinkManagerRegisterPCMMC DECODER_SYMBOL_NAME(VideoSinkManagerRegisterPCMMC)
#define VideoSinkManagerUnregister DECODER_SYMBOL_NAME(VideoSinkManagerUnregister)

bool VideoSinkManagerRegisterVDEC(const char *contextId);

bool VideoSinkManagerRegisterPCMMC(const char *contextId, const char *audioType);

bool VideoSinkManagerUnregister(const char *contextId);