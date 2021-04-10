#include "stream/api.h"
#include <stdbool.h>

bool platform_check_pi(PPLATFORM_INFO platform_info)
{
    platform_info->valid = true;
    platform_info->vrank = 35;
    platform_info->arank = 0;
    platform_info->maxBitrate = 20000;
    return true;
}
