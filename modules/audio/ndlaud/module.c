#include <stdlib.h>
#include <stdbool.h>

#include <NDL_directmedia.h>

#include "module/api.h"
#include "module/logging.h"

static bool ndl_initialized = false;
logvprintf_fn module_logvprintf;

MODULE_API bool audio_init_ndlaud(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        module_logvprintf = hctx->logvprintf;
    }
    applog_d("NDLAud", "init");
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0) {
        ndl_initialized = true;
    } else {
        ndl_initialized = false;
        applog_e("NDLAud", "Unable to initialize NDL: %s", NDL_DirectMediaGetError());
    }
    return ndl_initialized;
}

MODULE_API bool audio_check_ndlaud(PAUDIO_INFO ainfo) {
    if (!ndl_initialized) return false;
    ainfo->valid = true;
    return true;
}

MODULE_API void audio_finalize_ndlaud() {
    if (ndl_initialized) {
        NDL_DirectMediaQuit();
        ndl_initialized = false;
    }
}