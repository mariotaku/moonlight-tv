#pragma once
#include <NDL_directmedia.h>

#include "stream/module/api.h"

#define media_initialized PLUGIN_SYMBOL_NAME(audio_media_initialized)
#define media_loaded PLUGIN_SYMBOL_NAME(audio_media_loaded)
#define media_info PLUGIN_SYMBOL_NAME(audio_media_info)
#define media_reload PLUGIN_SYMBOL_NAME(audio_media_reload)
#define media_unload PLUGIN_SYMBOL_NAME(audio_media_unload)

extern bool media_initialized;
#if WEBOS_TARGET_VERSION_MAJOR >= 5
extern bool media_loaded;
extern NDL_DIRECTMEDIA_DATA_INFO media_info;
int media_reload();
void media_unload();
#endif