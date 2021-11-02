#include "stream/module/api.h"
#include "util/logging.h"

logvprintf_fn module_logvprintf;

bool audio_init_pulse(int argc, char *argv[], PHOST_CONTEXT hctx)
{
    if (hctx)
    {
        module_logvprintf = hctx->logvprintf;
    }
    return true;
}

bool audio_check_alsa(PAUDIO_INFO ainfo)
{
    ainfo->valid = true;
    ainfo->configuration = AUDIO_CONFIGURATION_51_SURROUND;
    return true;
}

void audio_finalize_alsa()
{
}
