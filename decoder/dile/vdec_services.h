#pragma once
#include "stream/platform.h"
#include "dile_platform.h"

#include <stdbool.h>
#include <pbnjson.h>

#define vdec_services_supported DECODER_SYMBOL_NAME(vdec_services_supported)
#define vdec_services_connect DECODER_SYMBOL_NAME(vdec_services_connect)
#define vdec_services_disconnect DECODER_SYMBOL_NAME(vdec_services_disconnect)
#define vdec_services_set_data DECODER_SYMBOL_NAME(vdec_services_set_data)
#define vdec_services_video_arrived DECODER_SYMBOL_NAME(vdec_services_video_arrived)

bool vdec_services_supported();

bool vdec_services_connect(const char *connId, const char *appId, jvalue_ref resources);

bool vdec_services_disconnect(const char *connId);

bool vdec_services_set_data(const char *connId, int framerate, int width, int height);

bool vdec_services_video_arrived();