#include "stream/api.h"

bool platform_check_pi(PPLATFORM_INFO pinfo)
{
    pinfo->valid = true;
    pinfo->accelerated = true;
    pinfo->audio = false;
    pinfo->maxBitrate = 20000;
    return true;
}
