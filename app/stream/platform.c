#include "platform.h"

#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"
#include "util.h"

typedef struct PLATFORM_DEFINITION
{
    const char *name;
    const char *id;
    const char *library;
    const PLATFORM_SYMBOLS *symbols;
} PLATFORM_DEFINITION;

static const PLATFORM_DEFINITION platform_definitions[FAKE + 1] = {
    {"No codec", NULL, NULL, NULL},
    {"SDL (SW codec)", "sdl", NULL, &platform_sdl},
    {"webOS NDL", "ndl", "ndl", NULL},
    {"NetCast Legacy", "lgnc", "lgnc", NULL},
    {"webOS SMP", "smp", "smp", NULL},
    {"webOS SMP", "smp_acb", "smp-acb", NULL},
    {"webOS DILE", "dile", "dile", NULL},
    {"webOS DILE", "dile_legacy", "dile-legacy", NULL},
    {"Raspberry Pi", "pi", "pi", NULL},
    {"Fake codec", NULL, NULL, NULL},
};
PLATFORM_INFO platform_states[FAKE + 1];

static const PLATFORM platform_orders[] = {
#if TARGET_WEBOS
    SMP, SMP_ACB, DILE_LEGACY, NDL, LGNC, SDL
#elif TARGET_RASPI
    PI, SDL
#else
    SDL
#endif
};
static const size_t platform_orders_len = sizeof(platform_orders) / sizeof(PLATFORM);

static void dlerror_log();
static bool checkinit(PLATFORM system, int argc, char *argv[]);

PLATFORM platform_current;

PLATFORM platform_init(const char *name, int argc, char *argv[])
{
    memset(platform_states, 0, sizeof(platform_states));
    bool std = strcmp(name, "auto") == 0;
    char libname[64];
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_DEFINITION pdef = platform_definitions[ptype];
        if (std || strcmp(name, pdef.id) == 0)
        {
            if (pdef.library)
            {
                snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.library);
                if (dlopen(libname, RTLD_NOW | RTLD_GLOBAL) == NULL)
                {
                    dlerror_log();
                    continue;
                }
            }
            checkinit(ptype, argc, argv);
        }
    }
    printf("Supported decoders: \n");
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platform_states[ptype];
        if (!pinfo.valid)
            continue;
        printf("%-10s\tvrank=%-2d\tarank=%-2d\thevc=%d\thdr=%d\tmaxbit=%d\n", platform_definitions[ptype].id,
               pinfo.vrank, pinfo.arank, pinfo.hevc, pinfo.hdr, pinfo.maxBitrate);
    }
    // Now all decoders has been loaded.
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platform_states[ptype];
        if (pinfo.valid)
            return ptype;
    }

    return NONE;
}

PDECODER_RENDERER_CALLBACKS platform_get_video(PLATFORM platform)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    if (pdef.symbols)
        return pdef.symbols->vdec;
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "decoder_callbacks_%s", platform_definitions[platform].id);
    PDECODER_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
    if (cb)
        return cb;
    return &decoder_callbacks_dummy;
}

PAUDIO_RENDERER_CALLBACKS platform_get_audio(PLATFORM platform, char *audio_device)
{
    unsigned int arank = 0;
    PLATFORM aplat = NONE;
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platform_states[ptype];
        if (!pinfo.valid)
            continue;
        if (pinfo.arank > arank)
        {
            arank = pinfo.arank;
            aplat = ptype;
        }
    }
    if (aplat != NONE)
    {
        PLATFORM_DEFINITION pdef = platform_definitions[aplat];
        if (pdef.symbols)
            return pdef.symbols->adec;
        char symbol[128];
        printf("audio render %s is preferred\n", platform_definitions[aplat]);
        snprintf(symbol, sizeof(symbol), "audio_callbacks_%s", platform_definitions[aplat].id);
        PAUDIO_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
        if (cb)
            return cb;
    }
    return &audio_callbacks_sdl;
}

PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(PLATFORM platform)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    if (pdef.symbols)
        return pdef.symbols->pres;
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "presenter_callbacks_%s", platform_definitions[platform].id);
    return dlsym(RTLD_DEFAULT, symbol);
}

static bool platform_init_simple(PLATFORM platform, int argc, char *argv[])
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    PLATFORM_INIT_FN fn;
    if (pdef.symbols)
    {
        fn = pdef.symbols->init;
    }
    else
    {
        char symbol[128];
        snprintf(symbol, sizeof(symbol), "platform_init_%s", platform_definitions[platform].id);
        fn = dlsym(RTLD_DEFAULT, symbol);
    }
    return !fn || fn(argc, argv);
}

static void platform_finalize_simple(PLATFORM platform)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    PLATFORM_FINALIZE_FN fn;
    if (pdef.symbols)
    {
        fn = pdef.symbols->finalize;
    }
    else
    {
        char symbol[128];
        snprintf(symbol, sizeof(symbol), "platform_finalize_%s", platform_definitions[platform].id);
        fn = dlsym(RTLD_DEFAULT, symbol);
    }
    if (fn)
        fn();
}

static bool platform_check_simple(PLATFORM platform)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    PLATFORM_CHECK_FN fn;
    if (pdef.symbols)
    {
        fn = pdef.symbols->check;
    }
    else
    {
        char fname[128];
        snprintf(fname, sizeof(fname), "platform_check_%s", platform_definitions[platform].id);
        fn = dlsym(RTLD_DEFAULT, fname);
    }
    PLATFORM_INFO *platform_info = &platform_states[platform];
    if (fn == NULL)
    {
        platform_info->valid = true;
        return true;
    }
    return fn(platform_info) && platform_info->valid;
}

bool checkinit(PLATFORM system, int argc, char *argv[])
{
    if (!platform_init_simple(system, argc, argv))
        return false;
    if (!platform_check_simple(system))
    {
        platform_finalize_simple(system);
        return false;
    }
    return true;
}

void platform_finalize(PLATFORM platform)
{
    printf("Supported decoders: \n");
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platform_states[ptype];
        if (!pinfo.valid)
            continue;
        platform_finalize_simple(ptype);
        printf("Finalized decoder %s\n", platform_definitions[ptype].id);
    }
}

const char *platform_name(PLATFORM system)
{
    return platform_definitions[system].name;
}

void dlerror_log()
{
    fprintf(stderr, "Unable to load platform library: %s\n", dlerror());
}
