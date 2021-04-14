#include "platform.h"

#include <assert.h>
#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "util.h"

PLATFORM_DEFINITION platform_definitions[PLATFORM_COUNT] = {
    {"No codec", NULL, NULL, NULL},
    {"FFMPEG (SW codec)", "ffmpeg-sw", "ffmpeg-sw", NULL},
    {"webOS NDL", "ndl", "ndl", NULL},
    {"NetCast Legacy", "lgnc", "lgnc", &platform_lgnc},
    {"webOS SMP", "smp", "smp", NULL},
    {"webOS SMP", "smp_acb", "smp-acb", NULL},
    {"webOS DILE", "dile", "dile", NULL},
    {"webOS DILE", "dile_legacy", "dile-legacy", NULL},
    {"Raspberry Pi", "pi", "pi", NULL},
    {"Fake codec", NULL, NULL, NULL},
};
PLATFORM_INFO platforms_info[PLATFORM_COUNT];
int platform_available_count = 0;

static void dlerror_log();
static bool checkinit(PLATFORM system, int argc, char *argv[]);

PLATFORM platform_default;

PLATFORM platforms_init(const char *name, int argc, char *argv[])
{
    memset(platforms_info, 0, sizeof(platforms_info));
    char libname[64];
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_DEFINITION pdef = platform_definitions[ptype];
        if (pdef.library && !(pdef.symbols && pdef.symbols->valid))
        {
            snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.library);
            // Lazy load to test if this library can be linked
            void *plib = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
            if (!plib)
            {
                dlerror_log();
                continue;
            }
            dlclose(plib);
            dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
        }
        if (checkinit(ptype, argc, argv))
        {
            platform_available_count++;
        }
    }
    printf("Supported decoders: \n");
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platforms_info[ptype];
        if (!pinfo.valid)
            continue;
        printf("%-10s\tvrank=%-2d\tarank=%-2d\thevc=%d\thdr=%d\tmaxbit=%d\n", platform_definitions[ptype].id,
               pinfo.vrank, pinfo.arank, pinfo.hevc, pinfo.hdr, pinfo.maxBitrate);
    }
    // Now all decoders has been loaded.
    // Return default platform
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platforms_info[ptype];
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

PAUDIO_RENDERER_CALLBACKS platform_get_audio(PLATFORM platform, char *audio_device, PLATFORM vplatform)
{
    PLATFORM aplat = platform_preferred_audio(vplatform);
    if (aplat != NONE)
    {
        PLATFORM_DEFINITION pdef = platform_definitions[aplat];
        if (pdef.symbols)
            return pdef.symbols->adec;
        char symbol[128];
        printf("audio render %s is preferred\n", platform_definitions[aplat].id);
        snprintf(symbol, sizeof(symbol), "audio_callbacks_%s", platform_definitions[aplat].id);
        PAUDIO_RENDERER_CALLBACKS cb = dlsym(RTLD_DEFAULT, symbol);
        if (cb)
            return cb;
    }
    return platform_sdl.adec;
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

PVIDEO_RENDER_CALLBACKS platform_get_render(PLATFORM platform)
{
    PLATFORM_DEFINITION pdef = platform_definitions[platform];
    if (pdef.symbols)
        return pdef.symbols->rend;
    char symbol[128];
    snprintf(symbol, sizeof(symbol), "render_callbacks_%s", platform_definitions[platform].id);
    return dlsym(RTLD_DEFAULT, symbol);
}

void platform_render_cleanup(PLATFORM platform)
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
        snprintf(symbol, sizeof(symbol), "render_cleanup_%s", platform_definitions[platform].id);
        fn = dlsym(RTLD_DEFAULT, symbol);
    }
    if (fn)
        fn();
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
    PLATFORM_INFO *pinfo = &platforms_info[platform];
    if (fn == NULL)
    {
        pinfo->valid = true;
        return true;
    }
    return fn(pinfo) && pinfo->valid;
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
        PLATFORM_INFO pinfo = platforms_info[ptype];
        if (!pinfo.valid)
            continue;
        platform_finalize_simple(ptype);
        printf("Finalized decoder %s\n", platform_definitions[ptype].id);
    }
}

PLATFORM platform_preferred_audio(PLATFORM vplatform)
{
    if (vplatform != NONE && !platforms_info[vplatform].vindependent)
        return vplatform;
    unsigned int arank = 0;
    PLATFORM aplat = NONE;
    for (int i = 0; i < platform_orders_len; i++)
    {
        PLATFORM ptype = platform_orders[i];
        PLATFORM_INFO pinfo = platforms_info[ptype];
        if (!pinfo.valid || !pinfo.aindependent)
            continue;
        if (pinfo.arank > arank)
        {
            arank = pinfo.arank;
            aplat = ptype;
        }
    }
    return aplat;
}

PLATFORM platform_by_id(const char *id)
{
    if (!id)
        return NONE;
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

bool platform_render_video()
{
    return false;
}

void dlerror_log()
{
    fprintf(stderr, "Unable to load platform library: %s\n", dlerror());
}
