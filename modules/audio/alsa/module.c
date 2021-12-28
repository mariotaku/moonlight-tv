#include "stream/module/api.h"
#include "util/logging.h"

logvprintf_fn module_logvprintf;

MODULE_API bool audio_init_pulse(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        module_logvprintf = hctx->logvprintf;
    }
    return true;
}

MODULE_API bool audio_check_alsa(PAUDIO_INFO ainfo) {
    ainfo->valid = true;
    ainfo->configuration = AUDIO_CONFIGURATION_51_SURROUND;
    return true;
}

MODULE_API void audio_finalize_alsa() {
}
