#include "platform.h"

#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"
#include "util.h"

static void dlerror_log();
static bool checkinit(enum platform system, int argc, char *argv[]);

enum platform platform_current;

enum platform platform_init(const char *name, int argc, char *argv[])
{
    bool std = strcmp(name, "auto") == 0;
#ifdef HAVE_SMP
    if (std || strcmp(name, "smp") == 0)
    {
        void *handle = dlopen("libmoonlight-smp.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle == NULL)
            dlerror_log();
        else if (!checkinit(SMP, argc, argv))
            fprintf(stderr, "SMP check failed\n");
        else
            return SMP;
    }
#endif
#ifdef HAVE_DILE
    if (std || strcmp(name, "dile") == 0)
    {
        void *handle = dlopen("libmoonlight-dile.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle == NULL)
            dlerror_log();
        else if (!checkinit(DILE, argc, argv))
            fprintf(stderr, "DILE check failed\n");
        else
            return DILE;
    }
#endif
#ifdef HAVE_DILE_LEGACY
    if (std || strcmp(name, "dile-legacy") == 0)
    {
        void *handle = dlopen("libmoonlight-dile-legacy.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle == NULL)
            dlerror_log();
        else if (!checkinit(DILE_LEGACY, argc, argv))
            fprintf(stderr, "DILE_LEGACY check failed\n");
        else
            return DILE_LEGACY;
    }
#endif
#ifdef HAVE_NDL
    if (std || strcmp(name, "ndl") == 0)
    {
        void *handle = dlopen("libmoonlight-ndl.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle == NULL)
            dlerror_log();
        else if (!checkinit(NDL, argc, argv))
            fprintf(stderr, "NDL check failed\n");
        else
            return NDL;
    }
#endif
#ifdef HAVE_LGNC
    if (std || strcmp(name, "lgnc") == 0)
    {
        void *handle = dlopen("libmoonlight-lgnc.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle == NULL)
            dlerror_log();
        else if (!checkinit(LGNC, argc, argv))
            fprintf(stderr, "LGNC check failed\n");
        else
            return LGNC;
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

#define get_decoder_callbacks_simple(name) (PDECODER_RENDERER_CALLBACKS) dlsym(RTLD_DEFAULT, "decoder_callbacks_" name)

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
    switch (system)
    {
#if HAVE_NDL
    case NDL:
        return get_decoder_callbacks_simple("ndl");
#endif
#if HAVE_DILE
    case DILE:
        return get_decoder_callbacks_simple("dile");
#endif
#if HAVE_DILE_LEGACY
    case DILE_LEGACY:
        return get_decoder_callbacks_simple("dile_legacy");
#endif
#if HAVE_LGNC
    case LGNC:
        return get_decoder_callbacks_simple("lgnc");
#endif
#if HAVE_SMP
    case SMP:
        return get_decoder_callbacks_simple("smp");
#endif
#if HAVE_PI
    case PI:
        return &decoder_callbacks_pi;
#endif
#if HAVE_MMAL
    case MMAL:
        return &decoder_callbacks_mmal;
#endif
    default:
#if HAVE_SDL && HAVE_FFMPEG
        return &decoder_callbacks_sdl;
#else
        return &decoder_callbacks_dummy;
#endif
    }
}

#define get_audio_callbacks_simple(name) (PAUDIO_RENDERER_CALLBACKS) dlsym(RTLD_DEFAULT, "audio_callbacks_" name)

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
    switch (system)
    {
#if HAVE_NDL
    case NDL:
        return get_audio_callbacks_simple("ndl");
#endif
#if HAVE_LGNC
    case LGNC:
        return get_audio_callbacks_simple("lgnc");
#endif
// #if HAVE_SMP
//     case SMP:
//         return get_audio_callbacks_simple("smp");
// #endif
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

typedef bool (*platform_init_fn)(int argc, char *argv[]);
typedef bool (*platform_check_fn)();
typedef void (*platform_finalize_fn)();

#define platform_init_simple(name, argc, argv) ((platform_init_fn)dlsym(RTLD_DEFAULT, "platform_init_" name))(argc, argv)
#define platform_finalize_simple(name) ((platform_finalize_fn)dlsym(RTLD_DEFAULT, "platform_finalize_" name))()
static bool platform_check_simple(const char *name)
{
    char fname[32];
    snprintf(fname, sizeof(fname), "platform_check_%s", name);
    platform_check_fn hnd = dlsym(RTLD_DEFAULT, fname);
    return hnd == NULL || hnd();
}

bool checkinit(enum platform system, int argc, char *argv[])
{
    switch (system)
    {
#ifdef HAVE_NDL
    case NDL:
        if (!platform_init_simple("ndl", argc, argv))
            return false;
        if (!platform_check_simple("ndl"))
        {
            platform_finalize(system);
            return false;
        }
        return true;
#endif
#ifdef HAVE_DILE
    case DILE:
        if (!platform_init_simple("dile", argc, argv))
            return false;
        if (!platform_check_simple("dile"))
        {
            platform_finalize(system);
            return false;
        }
        return true;
#endif
#ifdef HAVE_DILE_LEGACY
    case DILE_LEGACY:
        if (!platform_init_simple("dile_legacy", argc, argv))
            return false;
        if (!platform_check_simple("dile_legacy"))
        {
            platform_finalize(system);
            return false;
        }
        return true;
#endif
#ifdef HAVE_LGNC
    case LGNC:
        if (!platform_init_simple("lgnc", argc, argv))
            return false;
        if (!platform_check_simple("lgnc"))
        {
            platform_finalize(system);
            return false;
        }
        return true;
#endif
#ifdef HAVE_SMP
    case SMP:
        if (!platform_init_simple("smp", argc, argv))
            return false;
        if (!platform_check_simple("smp"))
        {
            platform_finalize(system);
            return false;
        }
        return true;
#endif
    }
    return false;
}

void platform_finalize(enum platform system)
{
    switch (system)
    {
#ifdef HAVE_NDL
    case NDL:
        platform_finalize_simple("ndl");
        break;
#endif
#ifdef HAVE_DILE
    case DILE:
        platform_finalize_simple("dile");
        break;
#endif
#ifdef HAVE_DILE
    case DILE_LEGACY:
        platform_finalize_simple("dile_legacy");
        break;
#endif
#ifdef HAVE_LGNC
    case LGNC:
        platform_finalize_simple("lgnc");
        break;
#endif
#ifdef HAVE_SMP
    case SMP:
        platform_finalize_simple("smp");
        break;
#endif
    default:
        break;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch-enum"
const char *platform_name(enum platform system)
{
    switch (system)
    {
    case NONE:
        return "NONE";
    case SDL:
        return "SDL (SW codec)";
    case X11:
        return "X11";
    case X11_VDPAU:
        return "X11_VDPAU";
    case X11_VAAPI:
        return "X11_VAAPI";
    case PI:
        return "Raspberry Pi";
    case MMAL:
        return "Raspberry Pi MMAL";
    case IMX:
        return "IMX";
    case AML:
        return "AML";
    case RK:
        return "RK";
    case NDL:
        return "webOS NDL";
    case LGNC:
        return "NetCast Legacy";
    case SMP:
        return "webOS SMP";
    case DILE:
        return "webOS DILE";
    case DILE_LEGACY:
        return "webOS DILE Legacy";
    case FAKE:
        return "Dummy Output";
    default:
        return "";
    }
}
#pragma GCC diagnostic pop

bool platform_is_software(enum platform system)
{
    switch (system)
    {
    case SDL:
        return true;
    default:
        return false;
    }
}

bool platform_supports_hevc(enum platform system)
{
    switch (system)
    {
    case DILE:
    case DILE_LEGACY:
    // case SMP:
        return true;
    default:
        return false;
    }
}

void dlerror_log()
{
    fprintf(stderr, "Unable to load platform library: %s\n", dlerror());
}