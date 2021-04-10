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
} PLATFORM_DEFINITION;

static const PLATFORM_DEFINITION platform_definitions[FAKE + 1] = {
    {"No codec", NULL, NULL},
    {"SDL (SW codec)", "sdl", NULL},
    {"webOS NDL", "ndl", "ndl"},
    {"NetCast Legacy", "lgnc", "lgnc"},
    {"webOS SMP", "smp", "smp"},
    {"webOS SMP", "smp_acb", "smp-acb"},
    {"webOS DILE", "dile", "dile"},
    {"webOS DILE", "dile_legacy", "dile-legacy"},
    {"Fake codec", NULL, NULL},
};
PLATFORM_INFO platform_states[FAKE + 1];

static const PLATFORM platform_orders[] = {SMP, SMP_ACB, DILE_LEGACY, NDL, LGNC, SDL};
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

PDECODER_RENDERER_CALLBACKS platform_get_video(PLATFORM system)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "decoder_callbacks_%s", platform_definitions[system].id);
    PDECODER_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
    if (cb)
        return cb;
    return &decoder_callbacks_dummy;
}

PAUDIO_RENDERER_CALLBACKS platform_get_audio(PLATFORM system, char *audio_device)
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
        char symbol[128];
        printf("audio render %s is preferred\n", platform_definitions[aplat]);
        snprintf(symbol, sizeof(symbol), "audio_callbacks_%s", platform_definitions[aplat].id);
        PAUDIO_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
        if (cb)
            return cb;
    }
    return &audio_callbacks_sdl;
}

PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(PLATFORM system)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "presenter_callbacks_%s", platform_definitions[system].id);
    return dlsym(RTLD_DEFAULT, symbol);
}

typedef bool (*platform_init_fn)(int argc, char *argv[]);
typedef bool (*platform_check_fn)();
typedef void (*platform_finalize_fn)();

static bool platform_init_simple(const char *suffix, int argc, char *argv[])
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "platform_init_%s", suffix);
    platform_init_fn fn = dlsym(RTLD_DEFAULT, symbol);
    return !fn || fn(argc, argv);
}

static void platform_finalize_simple(const char *suffix)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "platform_finalize_%s", suffix);
    platform_finalize_fn fn = dlsym(RTLD_DEFAULT, symbol);
    if (fn)
        fn();
}

static bool platform_check_simple(PLATFORM platform)
{
    PLATFORM_INFO *platform_info = &platform_states[platform];
    char fname[32];
    snprintf(fname, sizeof(fname), "platform_check_%s", platform_definitions[platform].id);
    platform_check_fn hnd = dlsym(RTLD_DEFAULT, fname);
    printf("%s => %p\n", fname, hnd);
    if (hnd == NULL)
    {
        platform_info->valid = true;
        return true;
    }
    return hnd(platform_info) && platform_info->valid;
}

bool checkinit(PLATFORM system, int argc, char *argv[])
{
    if (!platform_init_simple(platform_definitions[system].id, argc, argv))
        return false;
    if (!platform_check_simple(system))
    {
        platform_finalize_simple(platform_definitions[system].id);
        return false;
    }
    return true;
}

void platform_finalize(PLATFORM system)
{
    printf("Supported decoders: \n");
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platform_states[ptype];
        if (!pinfo.valid)
            continue;
        platform_finalize_simple(platform_definitions[ptype].id);
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