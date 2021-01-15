
#include "src/platform.h"

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"

#if OS_WEBOS
#include "platform/webos/app_init.h"
#endif

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
#if OS_WEBOS
    if (app_webos_ndl)
    {
        return &audio_callbacks_ndl;
    }
    if (app_webos_lgnc)
    {
        return &audio_callbacks_lgnc;
    }
    return NULL;
#else
    return &audio_callbacks_sdl;
#endif
}

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
#ifdef OS_WEBOS
    if (app_webos_ndl)
    {
        return &decoder_callbacks_ndl;
    }
    if (app_webos_lgnc)
    {
        return &decoder_callbacks_lgnc;
    }
    return NULL;
#else
    return &decoder_callbacks_sdl;
#endif
}