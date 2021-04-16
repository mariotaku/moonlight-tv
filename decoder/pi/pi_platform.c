#include "stream/api.h"

bool decoder_check_pi(PDECODER_INFO dinfo)
{
    dinfo->valid = true;
    dinfo->accelerated = true;
    dinfo->audio = false;
    dinfo->maxBitrate = 20000;
    return true;
}
