#pragma once
#include "dile_platform.h"

#include <stdbool.h>
#include <pbnjson.h>

bool DECODER_SYMBOL_NAME(vdec_services_supported)();

bool DECODER_SYMBOL_NAME(vdec_services_connect)(const char *connId, const char *appId, jvalue_ref resources);

bool DECODER_SYMBOL_NAME(vdec_services_disconnect)(const char *connId);

bool DECODER_SYMBOL_NAME(vdec_services_set_data)(const char *connId, int framerate, int width, int height);

bool DECODER_SYMBOL_NAME(vdec_services_video_arrived)();