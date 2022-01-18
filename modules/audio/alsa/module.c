#include "module/api.h"
#include "module/logging.h"

logvprintf_fn MODULE_LOGVPRINTF;

MODULE_API bool audio_init_pulse(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        MODULE_LOGVPRINTF = hctx->logvprintf;
    }
    return true;
}

MODULE_API bool audio_check_alsa(PAUDIO_INFO ainfo) {
    ainfo->valid = true;
    ainfo->configuration = AUDIO_CONFIGURATION_71_SURROUND;
    return true;
}

MODULE_API void audio_finalize_alsa() {
}
