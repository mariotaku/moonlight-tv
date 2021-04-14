#include <stddef.h>
#include "stream/api.h"

bool platform_check_sdl(PPLATFORM_INFO info)
{
    info->valid = true;
    info->vrank = 0;
    info->arank = 0;
    info->vindependent = true;
    info->aindependent = true;
    return true;
}

AUDIO_RENDERER_CALLBACKS audio_callbacks_sdl;
DECODER_RENDERER_CALLBACKS decoder_callbacks_sdl;