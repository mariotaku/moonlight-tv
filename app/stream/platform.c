
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
    bool std = strcmp(name, "auto") == 0;
#ifdef HAVE_NDL
    if (std || strcmp(name, "ndl") == 0)
    {
        void *handle = dlopen("libmoonlight-ndl.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle)
        {
            return NDL;
        }
    }
#endif
#ifdef HAVE_LGNC
    if (std || strcmp(name, "lgnc") == 0)
    {
        void *handle = dlopen("libmoonlight-lgnc.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle)
        {
            return LGNC;
        }
    }
#endif
#ifdef HAVE_PI
    if (std || strcmp(name, "pi") == 0)
    {
        void *handle = dlopen("libmoonlight-pi.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle != NULL && dlsym(RTLD_DEFAULT, "bcm_host_init") != NULL)
            return PI;
    }
#endif
#ifdef HAVE_SDL
    if (std || strcmp(name, "sdl") == 0)
        return SDL;
#endif
    if (strcmp(name, "fake") == 0)
        return FAKE;
    return 0;
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
#if HAVE_NDL
    case NDL:
        return (PDECODER_RENDERER_CALLBACKS)dlsym(RTLD_DEFAULT, "decoder_callbacks_ndl");
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
#if HAVE_NDL
    case NDL:
        return (PAUDIO_RENDERER_CALLBACKS)dlsym(RTLD_DEFAULT, "audio_callbacks_ndl");
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

typedef void (*platform_init_fn)(int argc, char *argv[]);
typedef void (*platform_finalize_fn)();

void platform_init(enum platform system, int argc, char *argv[])
{
    switch (system)
    {
#ifdef HAVE_NDL
    case NDL:
        ((platform_init_fn)dlsym(RTLD_DEFAULT, "platform_init_ndl"))(argc, argv);
        break;
#endif
    }
}

void platform_finalize(enum platform system)
{
    switch (system)
    {
#ifdef HAVE_NDL
    case NDL:
        ((platform_finalize_fn)dlsym(RTLD_DEFAULT, "platform_finalize_ndl"))();
        break;
#endif
    }
}