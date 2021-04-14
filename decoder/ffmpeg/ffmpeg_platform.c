#include <stddef.h>
#include "stream/api.h"

bool platform_check_ffmpeg(PPLATFORM_INFO info)
{
    info->valid = true;
    info->vrank = 0;
    info->arank = 0;
    info->vindependent = true;
    return true;
}