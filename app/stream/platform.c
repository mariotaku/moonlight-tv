#include "platform.h"

#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "audio/audio.h"
#include "video/video.h"
#include "util.h"

struct platform_definition
{
    const char *name;
    const char *symbol_suffix;
    const char *library;
};

static const struct platform_definition platform_definitions[FAKE + 1] = {
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

static const enum platform platform_orders[] = {SMP, SMP_ACB, DILE_LEGACY, NDL, LGNC, SDL};
static const size_t platform_orders_len = sizeof(platform_orders) / sizeof(enum platform);

static void dlerror_log();
static bool checkinit(enum platform system, int argc, char *argv[]);

enum platform platform_current;
PLATFORM_INFO platform_info;

enum platform platform_init(const char *name, int argc, char *argv[])
{
    bool std = strcmp(name, "auto") == 0;
    char libname[64];
    for (int i = 0; i < platform_orders_len; i++)
    {
        enum platform ptype = platform_orders[i];
        struct platform_definition pdef = platform_definitions[platform_orders[i]];
        printf("trying %s\n", pdef.library);
        if (std || strcmp(name, pdef.library) == 0)
        {
            snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.library);
            void *handle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
            if (handle == NULL)
                dlerror_log();
            else if (!checkinit(ptype, argc, argv))
                fprintf(stderr, "%s check failed\n", pdef.name);
            else
                return ptype;
        }
    }
    return NONE;
}

PDECODER_RENDERER_CALLBACKS platform_get_video(enum platform system)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "decoder_callbacks_%s", platform_definitions[system].symbol_suffix);
    PDECODER_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
    if (cb)
        return cb;
    return &decoder_callbacks_dummy;
}

PAUDIO_RENDERER_CALLBACKS platform_get_audio(enum platform system, char *audio_device)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "audio_callbacks_%s", platform_definitions[system].symbol_suffix);
    PAUDIO_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
    if (cb)
        return cb;
    return &audio_callbacks_sdl;
}

PVIDEO_PRESENTER_CALLBACKS platform_get_presenter(enum platform system)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "presenter_callbacks_%s", platform_definitions[system].symbol_suffix);
    return dlsym(RTLD_DEFAULT, symbol);
}

typedef bool (*platform_init_fn)(int argc, char *argv[]);
typedef bool (*platform_check_fn)();
typedef void (*platform_finalize_fn)();

static bool platform_init_simple(const char *suffix, int argc, char *argv[])
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "platform_init_%s", suffix);
    return ((platform_init_fn)dlsym(RTLD_DEFAULT, symbol))(argc, argv);
}

static void platform_finalize_simple(const char *suffix)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "platform_finalize_%s", suffix);
    ((platform_finalize_fn)dlsym(RTLD_DEFAULT, symbol))();
}

static bool platform_check_simple(const char *suffix)
{
    memset(&platform_info, 0, sizeof(PLATFORM_INFO));
    char fname[32];
    snprintf(fname, sizeof(fname), "platform_check_%s", suffix);
    platform_check_fn hnd = dlsym(RTLD_DEFAULT, fname);
    if (hnd == NULL)
    {
        platform_info.valid = true;
        return true;
    }
    bool good = hnd(&platform_info) && platform_info.valid;
    if (good)
    {
        printf("Platform %s check OK. audio=%d hevc=%d hdr=%d\n", suffix, platform_info.audio, platform_info.hevc,
               platform_info.hdr);
    }
    return good;
}

bool checkinit(enum platform system, int argc, char *argv[])
{
    switch (system)
    {
    default:
        if (!platform_init_simple(platform_definitions[system].symbol_suffix, argc, argv))
            return false;
        if (!platform_check_simple(platform_definitions[system].symbol_suffix))
        {
            platform_finalize(system);
            return false;
        }
        return true;
    }
    return false;
}

void platform_finalize(enum platform system)
{
    platform_finalize_simple(platform_definitions[system].symbol_suffix);
}

const char *platform_name(enum platform system)
{
    return platform_definitions[system].name;
}

void dlerror_log()
{
    fprintf(stderr, "Unable to load platform library: %s\n", dlerror());
}