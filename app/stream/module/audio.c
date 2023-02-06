#include "stream/platform.h"

#include <string.h>

#ifndef __WIN32

#include <dlfcn.h>

#endif

#include "util/logging.h"
#include "util/i18n.h"

static MODULE_LIB_DEFINITION _pulse_lib = {"pulse", "pulse"};
static MODULE_LIB_DEFINITION _alsa_lib = {"alsa", "alsa"};
static MODULE_LIB_DEFINITION _ndl_libs[] = {
#if DEBUG
        {"ndlaud_webos5", "ndlaud-webos5"},
#endif
        {"ndlaud", "ndlaud"},
};

bool audio_init_sdl(int argc, char *argv[], const HOST_CONTEXT *host);

bool audio_check_sdl(PAUDIO_INFO ainfo);

extern AUDIO_RENDERER_CALLBACKS audio_callbacks_sdl;

const static AUDIO_SYMBOLS audio_sdl = {
        true,
        &audio_init_sdl,
        &audio_check_sdl,
        NULL,
        &audio_callbacks_sdl,
};
#define AUDIO_SYMBOLS_SDL &audio_sdl


MODULE_DEFINITION audio_definitions[AUDIO_COUNT] = {
        {"Null",       "null", NULL,          0,                                                 NULL},
        {"SDL Audio",  "sdl",  NULL,          0,                                                 AUDIO_SYMBOLS_SDL},
        {"PulseAudio", "pulse",  &_pulse_lib, 1,                                                 NULL},
        {"ALSA",       "alsa",   &_alsa_lib,  1,                                                 NULL},
        {"NDL Audio",  "ndlaud", _ndl_libs,   sizeof(_ndl_libs) / sizeof(MODULE_LIB_DEFINITION), NULL,
#if FEATURE_CHECK_MODULE_OS_VERSION
                {OS_VERSION_MAKE(4, 0, 0)}
#endif
        },
};

const audio_config_entry_t audio_configs[] = {
        {AUDIO_CONFIGURATION_STEREO,      "stereo", translatable("Stereo")},
        {AUDIO_CONFIGURATION_51_SURROUND, "5.1ch",  translatable("5.1 Surround")},
        {AUDIO_CONFIGURATION_71_SURROUND, "7.1ch",  translatable("7.1 Surround")},
};
const size_t audio_config_len = sizeof(audio_configs) / sizeof(audio_config_entry_t);

AUDIO audio_current = AUDIO_NONE;
AUDIO_INFO audio_info;

AUDIO audio_by_id(const char *id) {
    if (!id || id[0] == 0 || strcmp(id, "auto") == 0)
        return AUDIO_AUTO;
    for (int i = 0; i < AUDIO_COUNT; i++) {
        MODULE_DEFINITION pdef = audio_definitions[i];
        if (!pdef.id)
            continue;
        if (strcmp(pdef.id, id) == 0)
            return i;
    }
    return AUDIO_SDL;
}

