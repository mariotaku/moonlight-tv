#include <stdlib.h>

#include "ndl_common.h"
#include "util/logging.h"

#define media_load_callback PLUGIN_SYMBOL_NAME(decoder_media_load_callback)

bool media_loaded = false;
NDL_DIRECTMEDIA_DATA_INFO media_info;

static void media_load_callback(int type, long long numValue, const char *strValue)
{
    applog_i("NDLAud", "MediaLoadCallback type=%d, numValue=%x, strValue=%p", type, numValue, strValue);
}

int media_reload()
{
    media_unload();
    int ret;
    if ((ret = NDL_DirectMediaInit(getenv("APPID"), NULL)) != 0)
    {
        return ret;
    }
    media_initialized = true;
    if ((ret = NDL_DirectMediaLoad(&media_info, media_load_callback)) != 0)
    {
        return ret;
    }
    media_loaded = true;
    return 0;
}

void media_unload()
{
    if (!media_loaded)
        return;
    NDL_DirectMediaUnload();
    media_loaded = false;
    NDL_DirectMediaQuit();
    media_initialized = false;
}