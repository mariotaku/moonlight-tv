#include "mmal_common.h"

logvprintf_fn module_logvprintf;

bool decoder_init_mmal(int argc, char *argv[], PHOST_CONTEXT hctx) {
    if (hctx) {
        module_logvprintf = hctx->logvprintf;
    }
    return true;
}

bool decoder_check_mmal(PDECODER_INFO dinfo) {
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->colorSpace = COLORSPACE_REC_709;
    dinfo->audio = false;
    dinfo->maxBitrate = 20000;
    return true;
}
