
#include "src/platform.h"

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
#ifdef OS_WEBOS
#ifndef WEBOS_LEGACY
    return &audio_callbacks_ndl;
#else
    return &audio_callbacks_lgnc;
#endif
#else
    return &audio_callbacks_sdl;
#endif
}

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
#ifdef OS_WEBOS
#ifndef WEBOS_LEGACY
    return &decoder_callbacks_ndl;
#else
    return &decoder_callbacks_lgnc;
#endif
#else
    return &decoder_callbacks_sdl;
#endif
}