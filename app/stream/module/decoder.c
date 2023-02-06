#include "config.h"
#include "stream/platform.h"
#include "util/os_info.h"

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
        {"cgl",  "cgl"},
        {"lgnc", "lgnc"},
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
        {"webOS NDL",         "ndl",    ndl_libs,    2, NULL,
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(4, 0, 0)}
#endif
        },
        {"NetCast Legacy",    "lgnc",   lgnc_libs,   2, NULL,
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(1, 0, 0),
                        OS_VERSION_MAKE(5, 0, 0)}
#endif
        },
        {"webOS SMP",         "smp",    smp_libs,    3, NULL,
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(3, 0, 0)}
#endif
        },
        {"webOS DILE",        "dile",   dile_libs,   2, NULL},
        {"Raspberry Pi",      "pi",     pi_libs,     1, NULL},
        {"MMAL",              "mmal",   mmal_libs,   1, NULL},
};
DECODER decoder_pref_requested;
DECODER decoder_current = DECODER_NONE;
DECODER_INFO decoder_info;

#ifndef __WIN32

#endif

PVIDEO_RENDER_CALLBACKS decoder_get_render() {
    return NULL;
}

int module_audio_configuration() {
    if (decoder_current == DECODER_NONE)
        return audio_info.configuration;
    if (decoder_info.audio)
        return decoder_info.audioConfig;
    return audio_info.configuration;
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

bool decoder_max_dimension(int *width, int *height) {
#if TARGET_WEBOS
    return SDL_webOSGetPanelResolution(width, height) == SDL_TRUE;
#else
    return false;
#endif
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

