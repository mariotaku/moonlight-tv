#include <stdlib.h>

#include "ndl_common.h"
#include "module/logging.h"

#define media_load_callback PLUGIN_SYMBOL_NAME(decoder_media_load_callback)

bool media_loaded = false;
NDL_DIRECTMEDIA_DATA_INFO media_info;
ndl_hdr_info_t hdr_info;

static void media_load_callback(int type, long long numValue, const char *strValue) {
    applog_i("NDL", "MediaLoadCallback type=%d, numValue=%x, strValue=%p", type, numValue, strValue);
}

int media_reload() {
    media_unload();
    int ret;
    if ((ret = NDL_DirectMediaInit(getenv("APPID"), NULL)) != 0) {
        return ret;
    }
    media_initialized = true;
    if ((ret = NDL_DirectMediaLoad(&media_info, media_load_callback)) != 0) {
        return ret;
    }
    if (hdr_info.supported) {
        NDL_DIRECTVIDEO_HDR_INFO v = hdr_info.value;
        NDL_DirectVideoSetHDRInfo(v.displayPrimariesX0, v.displayPrimariesY0, v.displayPrimariesX1,
                                  v.displayPrimariesY1, v.displayPrimariesX2, v.displayPrimariesY2,
                                  v.whitePointX, v.whitePointY, v.maxDisplayMasteringLuminance,
                                  v.minDisplayMasteringLuminance, v.maxContentLightLevel, v.maxPicAverageLightLevel);
    }
    media_loaded = true;
    return 0;
}

void media_unload() {
    if (!media_loaded)
        return;
    NDL_DirectMediaUnload();
    media_loaded = false;
    NDL_DirectMediaQuit();
    media_initialized = false;
}