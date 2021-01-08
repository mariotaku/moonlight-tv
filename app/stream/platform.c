
#include "platform.h"

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
#ifdef OS_WEBOS
#ifdef USE_NDL
    return &audio_callbacks_ndl;
#elif defined(USE_LGNCAPI)
    return &audio_callbacks_lgnc;
#endif
#else
#warning "No supported callbacks for this platform"
    return NULL;
#endif
}

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
#ifdef OS_WEBOS
#ifdef USE_NDL
    return &decoder_callbacks_ndl;
#elif defined(USE_LGNCAPI)
    return &decoder_callbacks_lgnc;
#endif
#else
#warning "No supported callbacks for this platform"
    return NULL;
#endif
}