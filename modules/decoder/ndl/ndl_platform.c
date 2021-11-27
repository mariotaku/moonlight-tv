#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <NDL_directmedia.h>

#include "ndl_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

bool media_initialized = false;
logvprintf_fn module_logvprintf;

#define decoder_init PLUGIN_SYMBOL_NAME(decoder_init)
#define decoder_post_init PLUGIN_SYMBOL_NAME(decoder_post_init)
#define decoder_check PLUGIN_SYMBOL_NAME(decoder_check)
#define decoder_finalize PLUGIN_SYMBOL_NAME(decoder_finalize)

MODULE_API bool decoder_init(int argc, char *argv[], PHOST_CONTEXT hctx) {
    if (hctx) {
        module_logvprintf = hctx->logvprintf;
    }
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0) {
        media_initialized = true;
    } else {
        media_initialized = false;
        applog_e("NDL", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
    return media_initialized;
}

MODULE_API bool decoder_check(PDECODER_INFO dinfo) {
    if (!media_initialized) return false;
    NDL_DIRECTVIDEO_DATA_INFO info = {.width = 1270, .height = 720};
    if (NDL_DirectVideoOpen(&info) != 0)
        return false;
    NDL_DirectVideoPlay(h264_test_frame, sizeof(h264_test_frame));
    NDL_DirectVideoClose();
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio = true;
    dinfo->colorSpace = COLORSPACE_REC_709;
    dinfo->maxBitrate = 50000;
    dinfo->suggestedBitrate = 35000;
    return true;
}

MODULE_API void decoder_finalize() {
    if (media_initialized) {
        NDL_DirectMediaQuit();
        media_initialized = false;
    }
}