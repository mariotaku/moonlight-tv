#pragma once
#include "stream/api.h"

#include <stdbool.h>
#include <pbnjson.h>

#define media_services_supported PLUGIN_SYMBOL_NAME(media_services_supported)
#define media_services_connect PLUGIN_SYMBOL_NAME(media_services_connect)
#define media_services_disconnect PLUGIN_SYMBOL_NAME(media_services_disconnect)
#define media_services_set_video_data PLUGIN_SYMBOL_NAME(media_services_set_video_data)
#define media_services_set_audio_data PLUGIN_SYMBOL_NAME(media_services_set_audio_data)
#define media_services_feed_arrived PLUGIN_SYMBOL_NAME(media_services_feed_arrived)

typedef enum MEDIA_SERVICES_TYPE_T {
    MEDIA_SERVICES_TYPE_AUDIO,
    MEDIA_SERVICES_TYPE_VIDEO,
} MEDIA_SERVICES_TYPE;

typedef struct MEDIA_SERVICES_CONTEXT *MEDIA_SERVICES_HANDLE;

bool media_services_supported();

MEDIA_SERVICES_HANDLE media_services_connect(const char *connId, const char *appId, jvalue_ref resources, MEDIA_SERVICES_TYPE type);

bool media_services_disconnect(MEDIA_SERVICES_HANDLE, const char *connId);

bool media_services_set_video_data(MEDIA_SERVICES_HANDLE, const char *connId, int framerate, int width, int height);

bool media_services_set_audio_data(MEDIA_SERVICES_HANDLE, const char *connId);

bool media_services_feed_arrived(MEDIA_SERVICES_HANDLE);