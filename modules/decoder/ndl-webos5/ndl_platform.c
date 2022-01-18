#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <NDL_directmedia_v2.h>

#include "ndl_common.h"
#include "module/logging.h"

bool media_initialized = false;
logvprintf_fn MODULE_LOGVPRINTF;

#define decoder_init PLUGIN_SYMBOL_NAME(decoder_init)
#define decoder_post_init PLUGIN_SYMBOL_NAME(decoder_post_init)
#define decoder_check PLUGIN_SYMBOL_NAME(decoder_check)
#define decoder_finalize PLUGIN_SYMBOL_NAME(decoder_finalize)

MODULE_API bool decoder_init(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        MODULE_LOGVPRINTF = hctx->logvprintf;
    }
    return true;
}

MODULE_API bool decoder_post_init(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0) {
        media_initialized = true;
    } else {
        media_initialized = false;
        applog_e("NDL", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
    memset(&media_info, 0, sizeof(media_info));
    memset(&hdr_info, 0, sizeof(hdr_info));
    return media_initialized;
}

MODULE_API bool decoder_check(PDECODER_INFO dinfo) {
    if (!media_initialized) return false;
//    NDL_DIRECTMEDIA_DATA_INFO info = {
//            .audio = {.type = 0},
//            .video = {.width = 1280, .height = 720, .type = NDL_VIDEO_TYPE_H264},
//    };
//    NDL_DirectMediaLoad(&info, NULL);
//    NDL_DirectVideoPlay(h264_test_frame, sizeof(h264_test_frame), 0);
//    NDL_DirectMediaUnload();
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio = true;
    dinfo->hevc = true;
#if DEBUG
    int support_multi_channel = 0;
    if (NDL_DirectAudioSupportMultiChannel(&support_multi_channel) == 0 && support_multi_channel) {
        dinfo->audioConfig = AUDIO_CONFIGURATION_51_SURROUND;
    } else {
        dinfo->audioConfig = AUDIO_CONFIGURATION_STEREO;
    }
#else
    dinfo->audioConfig = AUDIO_CONFIGURATION_STEREO;
#endif
    dinfo->hdr = DECODER_HDR_ALWAYS;
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