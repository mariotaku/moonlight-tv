#include "stream/api.h"

bool audio_init_alsa(int argc, char *argv[])
{
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
