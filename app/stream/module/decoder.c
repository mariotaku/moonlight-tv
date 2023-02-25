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

DECODER_INFO decoder_info = {NULL};


int module_audio_configuration() {
    // TODO: value from decoder
    return AUDIO_CONFIGURATION_STEREO;
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

