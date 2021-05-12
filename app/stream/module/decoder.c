#include "stream/platform.h"

#include <assert.h>
#include <dlfcn.h>
#include <string.h>

#include <Limelight.h>

#include "stream/util.h"
#include "util/logging.h"

#if TARGET_LGNC
extern DECODER_SYMBOLS decoder_lgnc;
#define LGNC_SYMBOLS &decoder_lgnc
#else
#define LGNC_SYMBOLS NULL
#endif

static MODULE_LIB_DEFINITION _ffmpeg_lib = {"ffmpeg", "ffmpeg"};
static MODULE_LIB_DEFINITION _ndl_libs[2] = {{"ndl_webos5", "ndl-webos5"}, {"ndl", "ndl"}};
static MODULE_LIB_DEFINITION _lgnc_lib = {"lgnc", "lgnc"};
static MODULE_LIB_DEFINITION _smp_libs[3] = {{"smp", "smp"}, {"smp_webos4", "smp-webos4"}, {"smp_webos3", "smp-webos3"}};
static MODULE_LIB_DEFINITION _dile_libs[2] = {{"dile", "dile"}, {"dile_legacy", "dile-legacy"}};
static MODULE_LIB_DEFINITION _pi_lib = {"pi", "pi"};

MODULE_DEFINITION decoder_definitions[DECODER_COUNT] = {
    {"No codec", NULL, NULL, 0, NULL},
    {"FFMPEG (SW codec)", "ffmpeg", &_ffmpeg_lib, 1, NULL},
    {"webOS NDL", "ndl", _ndl_libs, 2, NULL},
    {"NetCast Legacy", "lgnc", &_lgnc_lib, 1, LGNC_SYMBOLS},
    {"webOS SMP", "smp", _smp_libs, 3, NULL},
    {"webOS DILE", "dile", _dile_libs, 2, NULL},
    {"Raspberry Pi", "pi", &_pi_lib, 1, NULL},
#if DEBUG
    {"Null", "null", NULL, 0, NULL},
#endif
};
DECODER decoder_pref_requested;
DECODER decoder_current;
int decoder_current_libidx;
DECODER_INFO decoder_info;

static void dlerror_log();
static bool checkinit(DECODER system, int libidx, int argc, char *argv[]);

PAUDIO_RENDERER_CALLBACKS audio_get_callbacks(char *audio_device);

static bool decoder_try_init(DECODER ptype, int argc, char *argv[])
{
    char libname[64];
    MODULE_DEFINITION pdef = decoder_definitions[ptype];
    if (pdef.symbols.ptr && pdef.symbols.decoder->valid)
    {
        if (checkinit(ptype, -1, argc, argv))
        {
            decoder_current = ptype;
            decoder_current_libidx = -1;
            return true;
        }
    }
    else
    {
        for (int i = 0; i < pdef.liblen; i++)
        {
            snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.dynlibs[i].library);
            applog_d("APP", "Loading module %s", libname);
            // Lazy load to test if this library can be linked
            if (!dlopen(libname, RTLD_NOW | RTLD_GLOBAL))
            {
                dlerror_log();
                continue;
            }
            if (checkinit(ptype, i, argc, argv))
            {
                decoder_current = ptype;
                decoder_current_libidx = i;
                return true;
            }
        }
    }
    return false;
}

DECODER decoder_init(const char *name, int argc, char *argv[])
{
    DECODER platform = decoder_by_id(name);
    decoder_pref_requested = platform;
    if (platform != DECODER_AUTO)
    {
        // Has preferred value
        if (decoder_try_init(platform, argc, argv))
        {
            return platform;
        }
        return DECODER_NONE;
    }
    for (int i = 0; i < decoder_orders_len; i++)
    {
        DECODER ptype = decoder_orders[i];
        if (decoder_try_init(ptype, argc, argv))
        {
            return ptype;
        }
    }
    return DECODER_NONE;
}

static void *module_sym(char *fmt, DECODER platform, int libidx)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), fmt, decoder_definitions[platform].dynlibs[libidx].suffix);
    return dlsym(RTLD_DEFAULT, symbol);
}

PDECODER_RENDERER_CALLBACKS decoder_get_video()
{
    if (decoder_current == DECODER_NONE)
        return &decoder_callbacks_dummy;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->vdec;
    PDECODER_RENDERER_CALLBACKS cb = module_sym("decoder_callbacks_%s", decoder_current, decoder_current_libidx);
    if (cb)
        return cb;
    return &decoder_callbacks_dummy;
}

PAUDIO_RENDERER_CALLBACKS decoder_get_audio(char *audio_device)
{
    if (decoder_current == DECODER_NONE)
        return NULL;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->adec;
    PAUDIO_RENDERER_CALLBACKS cb = module_sym("audio_callbacks_%s", decoder_current, decoder_current_libidx);
    if (cb && decoder_info.audio)
        return cb;
    return NULL;
}

PVIDEO_PRESENTER_CALLBACKS decoder_get_presenter()
{
    if (decoder_current == DECODER_NONE)
        return NULL;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->pres;
    return module_sym("presenter_callbacks_%s", decoder_current, decoder_current_libidx);
}

PVIDEO_RENDER_CALLBACKS decoder_get_render()
{
    if (decoder_current == DECODER_NONE)
        return NULL;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->rend;
    return module_sym("render_callbacks_%s", decoder_current, decoder_current_libidx);
}

PAUDIO_RENDERER_CALLBACKS module_get_audio(char *audio_device)
{
    if (decoder_current == DECODER_NONE)
        return audio_get_callbacks(audio_device);
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->adec;
    PAUDIO_RENDERER_CALLBACKS cb = decoder_get_audio(audio_device);
    if (cb)
        return cb;
    return audio_get_callbacks(audio_device);
}

int module_audio_configuration()
{
    if (decoder_current == DECODER_NONE)
        return audio_info.configuration;
    if (decoder_info.audio)
        return decoder_info.audioConfig;
    return audio_info.configuration;
}

static bool decoder_init_simple(DECODER platform, int libidx, int argc, char *argv[])
{
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    MODULE_INIT_FN fn;
    if (pdef.symbols.ptr)
    {
        fn = pdef.symbols.decoder->init;
    }
    else
    {
        fn = module_sym("decoder_init_%s", platform, libidx);
    }
    return !fn || fn(argc, argv, &module_host_context);
}

static void decoder_finalize_simple(DECODER platform, int libidx)
{
    if (decoder_current == DECODER_NONE)
        // Nothing to finalize
        return;
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    MODULE_FINALIZE_FN fn;
    if (pdef.symbols.ptr)
    {
        fn = pdef.symbols.decoder->finalize;
    }
    else
    {
        fn = module_sym("decoder_finalize_%s", platform, libidx);
    }
    if (fn)
        fn();
}

static bool decoder_check_simple(DECODER platform, int libidx)
{
    memset(&decoder_info, 0, sizeof(decoder_info));
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    DECODER_CHECK_FN fn;
    if (pdef.symbols.ptr)
        fn = pdef.symbols.decoder->check;
    else
        fn = module_sym("decoder_check_%s", platform, libidx);
    if (fn == NULL)
    {
        decoder_info.valid = true;
        return true;
    }
    return fn(&decoder_info) && decoder_info.valid;
}

bool checkinit(DECODER system, int libidx, int argc, char *argv[])
{
    if (!decoder_init_simple(system, libidx, argc, argv))
        return false;
    if (!decoder_check_simple(system, libidx))
    {
        decoder_finalize_simple(system, libidx);
        return false;
    }
    return true;
}

void decoder_finalize()
{
    decoder_finalize_simple(decoder_current, decoder_current_libidx);
}

DECODER decoder_by_id(const char *id)
{
    if (!id || id[0] == 0 || strcmp(id, "auto") == 0)
        return DECODER_AUTO;
    for (int i = 0; i < DECODER_COUNT; i++)
    {
        MODULE_DEFINITION pdef = decoder_definitions[i];
        if (!pdef.id)
            continue;
        if (strcmp(pdef.id, id) == 0)
            return i;
    }
    return DECODER_NONE;
}

void dlerror_log()
{
    applog_w("APP", "Unable to load decoder module: %s", dlerror());
}
