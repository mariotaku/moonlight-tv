#include "stream/module/api.h"
#include "util/logging.h"

#include <stdio.h>
#include <pulse/simple.h>

logvprintf_fn module_logvprintf;

MODULE_API bool audio_init_pulse(int argc, char *argv[], const HOST_CONTEXT *hctx) {
    if (hctx) {
        module_logvprintf = hctx->logvprintf;
    }
    return true;
}

const static unsigned int channelChecks[] = {
        AUDIO_CONFIGURATION_71_SURROUND,
        AUDIO_CONFIGURATION_51_SURROUND,
        AUDIO_CONFIGURATION_STEREO,
};
const static int channelChecksCount = sizeof(channelChecks) / sizeof(unsigned int);

MODULE_API bool audio_check_pulse(PAUDIO_INFO ainfo) {
    for (int i = 0; i < channelChecksCount; i++) {
        pa_sample_spec spec = {
                .format = PA_SAMPLE_S16LE,
                .rate = 48000,
                .channels = CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(channelChecks[i]),
        };
        pa_simple *dev;
        int error;
        if (dev = pa_simple_new(NULL, "Moonlight TV", PA_STREAM_PLAYBACK, NULL, "Streaming", &spec, NULL, NULL,
                                &error)) {
            pa_simple_free(dev);
            ainfo->valid = true;
            ainfo->configuration = channelChecks[i];
            return true;
        }
    }
    return false;
}

MODULE_API void audio_finalize_pulse() {
}
