#include "stream/api.h"

bool audio_init_pulse(int argc, char *argv[])
{
    return true;
}

bool audio_check_pulse(PAUDIO_INFO ainfo)
{
    ainfo->valid = true;
    ainfo->configuration = AUDIO_CONFIGURATION_51_SURROUND;
    return true;
}

void audio_finalize_pulse()
{
}
