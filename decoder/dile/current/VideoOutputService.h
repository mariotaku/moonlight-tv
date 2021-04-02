#pragma once
#include <stdbool.h>

bool VideoOutputRegister(const char *contextId, const char *appId);

bool VideoOutputConnect(const char *contextId, int sourcePort);

bool VideoOutputSetVideoData(const char *contextId, int framerate, int width, int height);

bool VideoOutputSetDisplayWindow(const char *contextId, bool fullscreen, int x, int y, int width, int height);

bool VideoOutputBlankVideo(const char *contextId, bool blank);

bool VideoOutputDisconnect(const char *contextId);

bool VideoOutputUnregister(const char *contextId);

bool VideoOutputGetStatus();