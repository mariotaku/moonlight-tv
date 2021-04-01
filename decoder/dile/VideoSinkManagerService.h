#pragma once
#include <stdbool.h>

bool VideoSinkManagerRegister(const char *contextId);

bool VideoSinkManagerUnregister(const char *contextId);

bool AcbSetMediaVideoData(const char *contextId, int framerate, int width, int height);