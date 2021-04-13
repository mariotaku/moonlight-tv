#include "stream/api.h"

bool platform_check_pi(PPLATFORM_INFO pinfo)
{
    pinfo->valid = true;
    pinfo->vrank = 35;
    pinfo->arank = 0;
    pinfo->vindependent = true;
    pinfo->aindependent = true;
    pinfo->maxBitrate = 20000;
    return true;
}
