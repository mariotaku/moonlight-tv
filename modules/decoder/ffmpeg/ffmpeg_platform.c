#include "ffmpeg_symbols.h"

#include <libavcodec/avcodec.h>

#include "util/logging.h"

logvprintf_fn module_logvprintf;

MODULE_API bool decoder_init_ffmpeg(int argc, char *argv[], PHOST_CONTEXT hctx) {
    if (hctx) {
        module_logvprintf = hctx->logvprintf;
    }
    return true;
}

bool decoder_check_ffmpeg(PDECODER_INFO info) {
    if (avcodec_find_decoder(AV_CODEC_ID_HEVC)) {
        applog_i("FFMPEG", "Init with HEVC support");
        info->valid = true;
//        info->hevc = true;
    } else if (avcodec_find_decoder(AV_CODEC_ID_H264)) {
        applog_i("FFMPEG", "Init with H264 support");
        info->valid = true;
    } else {
        return false;
    }
    info->accelerated = false;
    info->audio = false;
    info->hasRenderer = true;

    return true;
}