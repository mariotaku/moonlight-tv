#include "config.h"
#include "stream/platform.h"
#include "os_info.h"

#include <string.h>

#include <Limelight.h>

#include "util/logging.h"
#include "symbols.h"

#if TARGET_WEBOS

#include <SDL.h>

#endif

MODULE_DEFINITION decoder_definitions[DECODER_COUNT] = {
        {"Null",              "null"},
        {"FFMPEG (SW codec)", "ffmpeg"},
        {"webOS NDL",         "ndl",
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(4, 0, 0)}
#endif
        },
        {"NetCast Legacy",    "lgnc",
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(1, 0, 0),
                        OS_VERSION_MAKE(5, 0, 0)}
#endif
        },
        {"webOS SMP",         "smp",
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(3, 0, 0)}
#endif
        },
        {"webOS DILE",        "dile"},
        {"Raspberry Pi",      "pi"},
        {"MMAL",              "mmal"},
};
DECODER decoder_pref_requested;
DECODER decoder_current = DECODER_NONE;
DECODER_INFO decoder_info = {NULL};


int module_audio_configuration() {
    // TODO: value from decoder
    return AUDIO_CONFIGURATION_STEREO;
}

DECODER decoder_by_id(const char *id) {
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
    int framerate = 0;
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

