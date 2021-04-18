#pragma once
#include <NDL_directmedia.h>

#include "stream/api.h"

#define media_loaded PLUGIN_SYMBOL_NAME(decoder_media_loaded)
#define media_info PLUGIN_SYMBOL_NAME(decoder_media_info)
#define media_load_callback PLUGIN_SYMBOL_NAME(decoder_media_load_callback)

#if NDL_WEBOS5
extern bool media_loaded;
extern NDL_DIRECTMEDIA_DATA_INFO media_info;
void media_load_callback(int type, long long numValue, const char * strValue);
#endif