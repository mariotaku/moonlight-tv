
#include "platform.h"

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"
#include "util.h"

#if OS_WEBOS
#include "platform/webos/app_init.h"
#include <string.h>
#endif

enum platform platform_check(char *name)
{
#if TARGET_RASPI
    return PI;
#elif OS_WEBOS
    if (app_webos_ndl)
    {
        return NDL;
    }
    else if (app_webos_lgnc)
    {
        return LGNC;
    }
    return NONE;
#elif TARGET_DESKTOP
    return SDL;
#endif
}

void platform_start(enum platform system)
{
    switch (system)
    {
#ifdef HAVE_AML
    case AML:
        blank_fb("/sys/class/graphics/fb0/blank", true);
        blank_fb("/sys/class/graphics/fb1/blank", true);
        break;
#endif
#if HAVE_PI || HAVE_MMAL
    case PI:
        blank_fb("/sys/class/graphics/fb0/blank", true);
        break;
#endif
    }
}

void platform_stop(enum platform system)
{
    switch (system)
    {
#ifdef HAVE_AML
    case AML:
        blank_fb("/sys/class/graphics/fb0/blank", false);
        blank_fb("/sys/class/graphics/fb1/blank", false);
        break;
#endif
#if HAVE_PI || HAVE_MMAL
    case PI:
        blank_fb("/sys/class/graphics/fb0/blank", false);
        break;
#endif
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
#if HAVE_PI
    case PI:
        return &decoder_callbacks_pi;
#endif
#if HAVE_MMAL
    case MMAL:
        return &decoder_callbacks_mmal;
#endif
#if HAVE_SDL && HAVE_FFMPEG
    case SDL:
        return &decoder_callbacks_sdl;
#endif
    default:
        return NULL;
    }
}

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
#if HAVE_SDL
    case SDL:
        return &audio_callbacks_sdl;
#endif
#if HAVE_PI
    case PI:
        return &audio_callbacks_omx;
#endif
    default:
#if HAVE_SDL
        return &audio_callbacks_sdl;
#else
        return NULL;
#endif
    }
}

PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(enum platform system)
{
    switch (system)
    {
#if HAVE_PI
    case PI:
        return &presenter_callbacks_pi;
#endif
    default:
        return NULL;
    }
}