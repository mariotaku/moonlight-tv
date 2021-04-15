#include <stddef.h>
#include "stream/api.h"

bool platform_check_ffmpeg(PPLATFORM_INFO info)
{
    info->valid = true;
    info->accelerated = false;
    info->audio = false;
    return true;
}