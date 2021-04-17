#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <NDL_directmedia.h>
#include "stream/api.h"

static bool ndl_initialized = false;

bool audio_init_ndlaud(int argc, char *argv[])
{
    if (NDL_DirectMediaInit(getenv("APPID"), NULL) == 0)
    {
        ndl_initialized = true;
    }
    else
    {
        ndl_initialized = false;
        fprintf(stderr, "Unable to initialize NDL: %s\n", NDL_DirectMediaGetError());
    }
    return ndl_initialized;
}

const static unsigned int channelChecks[] = {
    // AUDIO_CONFIGURATION_71_SURROUND,
    // AUDIO_CONFIGURATION_51_SURROUND,
    AUDIO_CONFIGURATION_STEREO,
};
const static int channelChecksCount = sizeof(channelChecks) / sizeof(unsigned int);

bool audio_check_ndlaud(PAUDIO_INFO dinfo)
{
    for (int i = 0; i < channelChecksCount; i++)
    {
        NDL_DIRECTAUDIO_DATA_INFO info = {
            .numChannel = CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(channelChecks[i]),
            .bitPerSample = 16,
            .nodelay = 1,
            .upperThreshold = 48,
            .lowerThreshold = 16,
            .channel = NDL_DIRECTAUDIO_CH_MAIN,
            .srcType = NDL_DIRECTAUDIO_SRC_TYPE_PCM,
            .samplingFreq = NDL_DIRECTAUDIO_SAMPLING_FREQ_OF(48000),
        };
        if (NDL_DirectAudioOpen(&info) == 0)
        {
            NDL_DirectAudioClose();
            dinfo->valid = true;
            dinfo->configuration = channelChecks[i];
            return true;
        }
    }
    return false;
}

void audio_finalize_ndlaud()
{
    if (ndl_initialized)
    {
        NDL_DirectMediaQuit();
        ndl_initialized = false;
    }
}