#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <NDL_directmedia_v2.h>

#include "ndl_common.h"
#include "module/logging.h"

bool media_initialized = false;
logvprintf_fn MODULE_LOGVPRINTF;

#define audio_init PLUGIN_SYMBOL_NAME(audio_init)
#define audio_check PLUGIN_SYMBOL_NAME(audio_check)
#define audio_finalize PLUGIN_SYMBOL_NAME(audio_finalize)

MODULE_API bool audio_init(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        MODULE_LOGVPRINTF = hctx->logvprintf;
    }
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0) {
        media_initialized = true;
    } else {
        media_initialized = false;
        applog_e("NDLAud", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
    memset(&media_info, 0, sizeof(media_info));
    return media_initialized;
}

MODULE_API bool audio_check(PAUDIO_INFO dinfo) {
    if (!media_initialized) return false;
    dinfo->valid = true;
#if DEBUG
    dinfo->configuration = AUDIO_CONFIGURATION_51_SURROUND;
#endif
    return true;
}

MODULE_API void audio_finalize() {
    if (media_initialized) {
        NDL_DirectMediaQuit();
        media_initialized = false;
    }
}