
#include "platform.h"

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"

#if OS_WEBOS
#include "platform/webos/app_init.h"
#include <string.h>
#endif

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
    switch (system)
    {
#if OS_LGNC || OS_WEBOS
    case LGNC:
        return &audio_callbacks_lgnc;
#endif
#if OS_WEBOS
    case NDL:
        return &audio_callbacks_ndl;
#endif
#if TARGET_DESKTOP
    case SDL:
        return &audio_callbacks_sdl;
#endif
    default:
        return NULL;
    }
}

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
    switch (system)
    {
#if OS_LGNC || OS_WEBOS
    case LGNC:
        return &decoder_callbacks_lgnc;
#endif
#if OS_WEBOS
    case NDL:
        return &decoder_callbacks_ndl;
#endif
#if TARGET_DESKTOP
    case SDL:
        return &decoder_callbacks_sdl;
#endif
    default:
        return NULL;
    }
}