#include "stream/platform.h"

#include <string.h>

#include <Limelight.h>

#ifndef __WIN32

#include <dlfcn.h>
#include "util/logging.h"

#endif

#include "symbols.h"

#if TARGET_WEBOS

#include <SDL.h>

#endif

static MODULE_LIB_DEFINITION ffmpeg_libs[] = {
        {"ffmpeg", "ffmpeg"}
};
static MODULE_LIB_DEFINITION ndl_libs[] = {
        {"ndl_webos5", "ndl-webos5"},
        {"ndl",        "ndl"}
};
static MODULE_LIB_DEFINITION lgnc_libs[] = {
        {"lgnc", "lgnc"}
};
static MODULE_LIB_DEFINITION smp_libs[] = {
        {"smp",        "smp"},
        {"smp_webos4", "smp-webos4"}
};
static MODULE_LIB_DEFINITION dile_libs[] = {
        {"dile",        "dile"},
        {"dile_legacy", "dile-legacy"}
};
static MODULE_LIB_DEFINITION pi_libs[] = {
        {"pi", "pi"}
};
static MODULE_LIB_DEFINITION mmal_libs[] = {
        {"mmal", "mmal"}
};

MODULE_DEFINITION decoder_definitions[DECODER_COUNT] = {
        {"Null",              "null", NULL,          0, NULL},
        {"FFMPEG (SW codec)", "ffmpeg", ffmpeg_libs, 1, DECODER_SYMBOLS_FFMPEG},
        {"webOS NDL",         "ndl",    ndl_libs,    2, NULL},
        {"NetCast Legacy",    "lgnc",   lgnc_libs,   1, NULL},
        {"webOS SMP",         "smp",    smp_libs,    3, NULL},
        {"webOS DILE",        "dile",   dile_libs,   2, NULL},
        {"Raspberry Pi",      "pi",     pi_libs,     1, NULL},
        {"MMAL",              "mmal",   mmal_libs,   1, NULL},
};
DECODER decoder_pref_requested;
DECODER decoder_current = DECODER_NONE;
int decoder_current_libidx;
DECODER_INFO decoder_info;

static void dlerror_log();

static bool checkinit(DECODER system, int libidx, int argc, char *argv[]);

static bool decoder_init_simple(DECODER platform, int libidx, int argc, char *argv[]);

static bool decoder_post_init_simple(DECODER platform, int libidx, int argc, char *argv[]);

static void decoder_finalize_simple(DECODER platform, int libidx);

PAUDIO_RENDERER_CALLBACKS audio_get_callbacks(const char *audio_device);

static bool decoder_try_init(DECODER ptype, int argc, char *argv[]) {
    MODULE_DEFINITION pdef = decoder_definitions[ptype];
    if (pdef.symbols.ptr && pdef.symbols.decoder->valid) {
        if (decoder_init_simple(ptype, -1, argc, argv)) {
            decoder_current = ptype;
            decoder_current_libidx = -1;
            return true;
        }
    } else {
#ifndef __WIN32
        char libname[64];
        for (int i = 0; i < pdef.liblen; i++) {
            snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.dynlibs[i].library);
            applog_d("APP", "Loading module %s", libname);
            // Lazy load to test if this library can be linked
            if (!dlopen(libname, RTLD_NOW | RTLD_GLOBAL)) {
                dlerror_log();
                continue;
            }
            if (decoder_init_simple(ptype, i, argc, argv)) {
                decoder_current = ptype;
                decoder_current_libidx = i;
                return true;
            }
        }
#endif
    }
    decoder_current = DECODER_EMPTY;
    return false;
}

DECODER decoder_init(const char *name, int argc, char *argv[]) {
    DECODER platform = decoder_by_id(name);
    decoder_pref_requested = platform;
    if (platform != DECODER_AUTO) {
        // Has preferred value
        if (decoder_try_init(platform, argc, argv)) {
            return platform;
        }
        return DECODER_NONE;
    }
    for (int i = 0; i < decoder_orders_len; i++) {
        DECODER ptype = decoder_orders[i];
        if (decoder_try_init(ptype, argc, argv)) {
            return ptype;
        }
    }
    return DECODER_NONE;
}

bool decoder_post_init(DECODER decoder, int libidx, int argc, char *argv[]) {
    if (decoder >= DECODER_COUNT) {
        return false;
    }
    return decoder_post_init_simple(decoder, libidx, argc, argv);
}

#ifndef __WIN32
static void *module_sym(char *fmt, DECODER platform, int libidx) {
    MODULE_DEFINITION def = decoder_definitions[platform];
    if (!def.dynlibs) return NULL;
    char symbol[128];
    snprintf(symbol, sizeof(symbol), fmt, def.dynlibs[libidx].suffix);
    return dlsym(RTLD_DEFAULT, symbol);
}
#endif

PDECODER_RENDERER_CALLBACKS decoder_get_video() {
    if (decoder_current == DECODER_NONE)
        return &decoder_callbacks_dummy;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->vdec;
#ifndef __WIN32
    PDECODER_RENDERER_CALLBACKS cb = module_sym("decoder_callbacks_%s", decoder_current, decoder_current_libidx);
    if (cb)
        return cb;
#endif
    return &decoder_callbacks_dummy;
}

PAUDIO_RENDERER_CALLBACKS decoder_get_audio(const char *audio_device) {
    if (decoder_current == DECODER_NONE)
        return NULL;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->adec;
#ifndef __WIN32
    PAUDIO_RENDERER_CALLBACKS cb = module_sym("audio_callbacks_%s", decoder_current, decoder_current_libidx);
    if (cb && decoder_info.audio)
        return cb;
#endif
    return NULL;
}

PVIDEO_PRESENTER_CALLBACKS decoder_get_presenter() {
    if (decoder_current == DECODER_NONE)
        return NULL;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->pres;
#ifndef __WIN32
    return module_sym("presenter_callbacks_%s", decoder_current, decoder_current_libidx);
#else
    return NULL;
#endif
}

PVIDEO_RENDER_CALLBACKS decoder_get_render() {
    if (decoder_current == DECODER_NONE)
        return NULL;
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->rend;
#ifndef __WIN32
    return module_sym("render_callbacks_%s", decoder_current, decoder_current_libidx);
#else
    return NULL;
#endif
}

PAUDIO_RENDERER_CALLBACKS module_get_audio(const char *audio_device) {
    if (audio_current > 0)
        return audio_get_callbacks(audio_device);
    MODULE_DEFINITION pdef = decoder_definitions[decoder_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.decoder->adec;
    PAUDIO_RENDERER_CALLBACKS cb = decoder_get_audio(audio_device);
    if (cb)
        return cb;
    return audio_get_callbacks(audio_device);
}

int module_audio_configuration() {
    if (decoder_current == DECODER_NONE)
        return audio_info.configuration;
    if (decoder_info.audio)
        return decoder_info.audioConfig;
    return audio_info.configuration;
}

static bool decoder_init_simple(DECODER platform, int libidx, int argc, char *argv[]) {
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    MODULE_INIT_FN fn = NULL;
    if (pdef.symbols.ptr) {
        fn = pdef.symbols.decoder->init;
    }
#ifdef __WIN32
    (void) libidx;
#else
    else {
        fn = module_sym("decoder_init_%s", platform, libidx);
    }
#endif
    return !fn || fn(argc, argv, &module_host_context);
}

static bool decoder_post_init_simple(DECODER platform, int libidx, int argc, char *argv[]) {
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    MODULE_INIT_FN fn = NULL;
    if (pdef.symbols.ptr) {
        fn = pdef.symbols.decoder->post_init;
    }
#ifdef __WIN32
    (void) libidx;
#else
    else {
            fn = module_sym("decoder_post_init_%s", platform, libidx);
        }
#endif
    return !fn || fn(argc, argv, &module_host_context);
}

bool decoder_check_info(DECODER platform, int libidx) {
    memset(&decoder_info, 0, sizeof(decoder_info));
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    DECODER_CHECK_FN fn = NULL;
    if (pdef.symbols.ptr) {
        fn = pdef.symbols.decoder->check;
    }
#ifdef __WIN32
    (void) libidx;
#else
    else {
            fn = module_sym("decoder_check_%s", platform, libidx);
        }
#endif
    if (fn == NULL) {
        decoder_info.valid = true;
        return true;
    }
    return fn(&decoder_info) && decoder_info.valid;
}

static void decoder_finalize_simple(DECODER platform, int libidx) {
    if (decoder_current == DECODER_NONE)
        // Nothing to finalize
        return;
    MODULE_DEFINITION pdef = decoder_definitions[platform];
    MODULE_FINALIZE_FN fn = NULL;
    if (pdef.symbols.ptr) {
        fn = pdef.symbols.decoder->finalize;
    }
#ifdef __WIN32
    (void) libidx;
#else
    else {
        fn = module_sym("decoder_finalize_%s", platform, libidx);
    }
#endif
    if (fn)
        fn();
}

void decoder_finalize() {
    decoder_finalize_simple(decoder_current, decoder_current_libidx);
}

DECODER decoder_by_id(const char *id) {
    if (!id || id[0] == 0 || strcmp(id, "auto") == 0)
        return DECODER_AUTO;
    for (int i = 0; i < DECODER_COUNT; i++) {
        MODULE_DEFINITION pdef = decoder_definitions[i];
        if (!pdef.id)
            continue;
        if (strcmp(pdef.id, id) == 0)
            return i;
    }
    return DECODER_NONE;
}

int decoder_max_framerate() {
    int framerate = decoder_info.maxFramerate;
#if TARGET_WEBOS
    int panel_fps = 0;
    SDL_webOSGetRefreshRate(&panel_fps);
    if (framerate <= 0) {
        framerate = panel_fps;
    } else if (panel_fps > 0) {
        framerate = SDL_min(panel_fps, framerate);
    }
#endif
    return framerate;
}

void dlerror_log() {
#ifndef __WIN32
    applog_w("APP", "Unable to load decoder module: %s", dlerror());
#endif
}
