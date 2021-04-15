#include "platform.h"

#include <assert.h>
#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "util.h"

#if TARGET_LGNC
extern PLATFORM_SYMBOLS platform_lgnc;
#define LGNC_SYMBOLS &platform_lgnc
#else
#define LGNC_SYMBOLS NULL
#endif

static PLATFORM_DYNLIB_DEFINITION _ffmpeg_lib = {"ffmpeg", "ffmpeg"};
static PLATFORM_DYNLIB_DEFINITION _ndl_lib = {"ndl", "ndl"};
static PLATFORM_DYNLIB_DEFINITION _lgnc_lib = {"lgnc", "lgnc"};
static PLATFORM_DYNLIB_DEFINITION _smp_libs[2] = {{"smp", "smp"}, {"smp_acb", "smp-acb"}};
static PLATFORM_DYNLIB_DEFINITION _dile_libs[2] = {{"dile", "dile"}, {"dile_legacy", "dile-legacy"}};
static PLATFORM_DYNLIB_DEFINITION _pi_lib = {"pi", "pi"};

PLATFORM_DEFINITION platform_definitions[PLATFORM_COUNT] = {
    {"No codec", NULL, NULL, 0, NULL},
    {"FFMPEG (SW codec)", "ffmpeg", &_ffmpeg_lib, 1, NULL},
    {"webOS NDL", "ndl", &_ndl_lib, 1, NULL},
    {"NetCast Legacy", "lgnc", &_lgnc_lib, 1, LGNC_SYMBOLS},
    {"webOS SMP", "smp", _smp_libs, 2, NULL},
    {"webOS DILE", "dile", _dile_libs, 2, NULL},
    {"Raspberry Pi", "pi", &_pi_lib, 1, NULL},
};
PLATFORM platform_pref_requested;
PLATFORM platform_current;
int platform_current_libidx;
PLATFORM_INFO platform_info;

extern AUDIO_RENDERER_CALLBACKS audio_callbacks_sdl;

static void dlerror_log();
static bool checkinit(PLATFORM system, int libidx, int argc, char *argv[]);

static bool platform_try_init(PLATFORM ptype, int argc, char *argv[])
{
    char libname[64];
    PLATFORM_DEFINITION pdef = platform_definitions[ptype];
    if (pdef.symbols && pdef.symbols->valid)
    {
        if (checkinit(ptype, -1, argc, argv))
        {
            platform_current = ptype;
            platform_current_libidx = -1;
            return true;
        }
    }
    else
    {
        for (int i = 0; i < pdef.liblen; i++)
        {
            snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.dynlibs[i].library);
            printf("Try init %s\n", libname);
            // Lazy load to test if this library can be linked
            if (!dlopen(libname, RTLD_NOW | RTLD_GLOBAL))
            {
                dlerror_log();
                continue;
            }
            if (checkinit(ptype, i, argc, argv))
            {
                platform_current = ptype;
                platform_current_libidx = i;
                return true;
            }
        }
    }
    return false;
}

PLATFORM platform_init(const char *name, int argc, char *argv[])
{
    PLATFORM platform = platform_by_id(name);
    platform_pref_requested = platform;
    if (platform != AUTO)
    {
        // Has preferred value
        if (platform_try_init(platform, argc, argv))
        {
            return platform;
        }
        return NONE;
    }
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        if (platform_try_init(ptype, argc, argv))
        {
            return ptype;
        }
    }
    return NONE;
}

static void *platform_dlsym(char *fmt, PLATFORM platform, int libidx)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), fmt, platform_definitions[platform].dynlibs[libidx].suffix);
    return dlsym(RTLD_DEFAULT, symbol);
}

PDECODER_RENDERER_CALLBACKS platform_get_video()
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform_current];
    if (pdef.symbols)
        return pdef.symbols->vdec;
    PDECODER_RENDERER_CALLBACKS cb = platform_dlsym("decoder_callbacks_%s", platform_current, platform_current_libidx);
    if (cb)
        return cb;
    return &decoder_callbacks_dummy;
}

PAUDIO_RENDERER_CALLBACKS platform_get_audio(char *audio_device)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform_current];
    if (pdef.symbols)
        return pdef.symbols->adec;
    PAUDIO_RENDERER_CALLBACKS cb = platform_dlsym("audio_callbacks_%s", platform_current, platform_current_libidx);
    if (cb && platform_info.arank)
        return cb;
    return &audio_callbacks_sdl;
}

PVIDEO_PRESENTER_CALLBACKS platform_get_presenter()
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform_current];
    if (pdef.symbols)
        return pdef.symbols->pres;
    return platform_dlsym("presenter_callbacks_%s", platform_current, platform_current_libidx);
}

PVIDEO_RENDER_CALLBACKS platform_get_render()
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform_current];
    if (pdef.symbols)
        return pdef.symbols->rend;
    return platform_dlsym("render_callbacks_%s", platform_current, platform_current_libidx);
}

static bool platform_init_simple(PLATFORM platform, int libidx, int argc, char *argv[])
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    PLATFORM_INIT_FN fn;
    if (pdef.symbols)
    {
        fn = pdef.symbols->init;
    }
    else
    {
        fn = platform_dlsym("platform_init_%s", platform, libidx);
    }
    return !fn || fn(argc, argv);
}

static void platform_finalize_simple(PLATFORM platform, int libidx)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    PLATFORM_FINALIZE_FN fn;
    if (pdef.symbols)
    {
        fn = pdef.symbols->finalize;
    }
    else
    {
        fn = platform_dlsym("platform_finalize_%s", platform, libidx);
    }
    if (fn)
        fn();
}

static bool platform_check_simple(PLATFORM platform, int libidx)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    PLATFORM_CHECK_FN fn;
    if (pdef.symbols)
        fn = pdef.symbols->check;
    else
        fn = platform_dlsym("platform_check_%s", platform, libidx);
    if (fn == NULL)
    {
        platform_info.valid = true;
        return true;
    }
    return fn(&platform_info) && platform_info.valid;
}

bool checkinit(PLATFORM system, int libidx, int argc, char *argv[])
{
    if (!platform_init_simple(system, libidx, argc, argv))
        return false;
    if (!platform_check_simple(system, libidx))
    {
        platform_finalize_simple(system, libidx);
        return false;
    }
    return true;
}

void platform_finalize()
{
    platform_finalize_simple(platform_current, platform_current_libidx);
}

PLATFORM platform_by_id(const char *id)
{
    if (!id || id[0] == 0 || strcmp(id, "auto") == 0)
        return AUTO;
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        PLATFORM_DEFINITION pdef = platform_definitions[i];
        if (!pdef.id)
            continue;
        if (strcmp(pdef.id, id) == 0)
            return i;
    }
    return NONE;
}

void dlerror_log()
{
    fprintf(stderr, "Unable to load platform library: %s\n", dlerror());
}
