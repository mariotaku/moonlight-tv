#pragma once
#include <stdbool.h>

bool VideoSinkManagerRegisterVDEC(const char *contextId);

bool VideoSinkManagerRegisterPCMMC(const char *contextId, const char *audioType);

bool VideoSinkManagerUnregister(const char *contextId);