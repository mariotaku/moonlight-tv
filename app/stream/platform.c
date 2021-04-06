#include "platform.h"

#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"
#include "util.h"

static const char *platform_names[FAKE + 1] = {
    "",
    "sdl",
    "x11",
    "x11_vdpau",
    "x11_vaapi",
    "pi",
    "mmal",
    "imx",
    "aml",
    "rk",
    "ndl",
    "lgnc",
    "smp",
    "smp_acb",
    "dile",
    "dile_legacy",
    NULL,
};

static void dlerror_log();
static bool checkinit(enum platform system, int argc, char *argv[]);

enum platform platform_current;
PLATFORM_INFO platform_info;

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
#ifdef HAVE_SMP_ACB
    if (std || strcmp(name, "smp-acb") == 0)
    {
        void *handle = dlopen("libmoonlight-smp-acb.so", RTLD_NOW | RTLD_GLOBAL);
        if (handle == NULL)
            dlerror_log();
        else if (!checkinit(SMP_ACB, argc, argv))
            fprintf(stderr, "SMP check failed\n");
        else
            return SMP_ACB;
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

static PDECODER_RENDERER_CALLBACKS get_decoder_callbacks_simple(const char *name)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "decoder_callbacks_%s", name);
    return dlsym(RTLD_DEFAULT, symbol);
}

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
    switch (system)
    {
    SDL:
#if HAVE_SDL && HAVE_FFMPEG
        return &decoder_callbacks_sdl;
        // If no FFMPEG, fall through
#endif
    FAKE:
        return &decoder_callbacks_dummy;
    default:
        return get_decoder_callbacks_simple(platform_names[system]);
    }
}

static PAUDIO_RENDERER_CALLBACKS get_audio_callbacks_simple(const char *name)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "audio_callbacks_%s", name);
    return dlsym(RTLD_DEFAULT, symbol);
}

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
    switch (system)
    {
    case NDL:
    case LGNC:
        return get_audio_callbacks_simple(platform_names[system]);
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

static bool platform_init_simple(const char *name, int argc, char *argv[])
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "platform_init_%s", name);
    return ((platform_init_fn)dlsym(RTLD_DEFAULT, symbol))(argc, argv);
}

static void platform_finalize_simple(const char *name)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "platform_finalize_%s", name);
    ((platform_finalize_fn)dlsym(RTLD_DEFAULT, symbol))();
}

static bool platform_check_simple(const char *name)
{
    memset(&platform_info, 0, sizeof(PLATFORM_INFO));
    char fname[32];
    snprintf(fname, sizeof(fname), "platform_check_%s", name);
    platform_check_fn hnd = dlsym(RTLD_DEFAULT, fname);
    if (hnd == NULL)
    {
        platform_info.valid = true;
        return true;
    }
    bool good = hnd(&platform_info) && platform_info.valid;
    if (good)
    {
        printf("Platform %s check OK. audio=%d hevc=%d hdr=%d\n", name, platform_info.audio, platform_info.hevc,
               platform_info.hdr);
    }
    return good;
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
#ifdef HAVE_SMP_ACB
    case SMP_ACB:
        if (!platform_init_simple("smp_acb", argc, argv))
            return false;
        if (!platform_check_simple("smp_acb"))
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
    default:
        platform_finalize_simple(platform_names[system]);
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
    case SMP_ACB:
        return "webOS SMP";
    case DILE:
    case DILE_LEGACY:
        return "webOS DILE";
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

void dlerror_log()
{
    fprintf(stderr, "Unable to load platform library: %s\n", dlerror());
}