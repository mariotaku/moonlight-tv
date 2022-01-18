#include "mmal_common.h"

logvprintf_fn MODULE_LOGVPRINTF;

MODULE_API bool decoder_init_mmal(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        MODULE_LOGVPRINTF = hctx->logvprintf;
    }
    return true;
}

MODULE_API bool decoder_check_mmal(PDECODER_INFO dinfo) {
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->colorSpace = COLORSPACE_REC_709;
    dinfo->audio = false;
    dinfo->maxBitrate = 20000;
    return true;
}
