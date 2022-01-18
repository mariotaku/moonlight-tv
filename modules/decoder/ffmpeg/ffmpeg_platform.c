#include "module/api.h"
#include "module/logging.h"

#include "ffmpeg_symbols.h"

#include <libavcodec/avcodec.h>

logvprintf_fn MODULE_LOGVPRINTF;

MODULE_API bool decoder_init_ffmpeg(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        MODULE_LOGVPRINTF = hctx->logvprintf;
    }
    return true;
}

MODULE_API bool decoder_check_ffmpeg(PDECODER_INFO info) {
    if (avcodec_find_decoder(AV_CODEC_ID_HEVC)) {
        applog_i("FFMPEG", "Init with HEVC support");
        info->valid = true;
        info->hevc = true;
    } else if (avcodec_find_decoder(AV_CODEC_ID_H264)) {
        applog_i("FFMPEG", "Init with H264 support");
        info->valid = true;
    } else {
        return false;
    }
    info->accelerated = false;
    info->audio = false;
    info->hasRenderer = true;
    info->suggestedBitrate = 50000;

    return true;
}