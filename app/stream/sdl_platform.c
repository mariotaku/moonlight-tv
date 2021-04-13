#include "platform.h"

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

PLATFORM_SYMBOLS platform_sdl = {
    NULL,
    &platform_check_sdl,
    NULL,
    &audio_callbacks_sdl,
    &decoder_callbacks_sdl,
    NULL,
    &render_callbacks_sdl,
};