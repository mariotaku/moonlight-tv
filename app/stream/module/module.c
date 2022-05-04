#include "app.h"
#include "stream/platform.h"
#include "util/logging.h"

const HOST_CONTEXT module_host_context = {
        .logvprintf = (void *) app_logvprintf,
        .seterror = module_seterror,
};

static char module_error[1024];

void module_init(int argc, char *argv[]) {
    // LGNC requires init before window created, don't put this after app_window_create!
    decoder_init(app_configuration->decoder, argc, argv);
    // If decoder_info is valid, initialize audio as well
    if (decoder_check_info(decoder_current, decoder_current_libidx)) {
        audio_init(app_configuration->audio_backend, argc, argv);
    }
}

void module_post_init(int argc, char *argv[]) {
    if (decoder_current >= DECODER_FIRST && decoder_current < DECODER_COUNT) {
        decoder_post_init(decoder_current, decoder_current_libidx, argc, argv);
        // If decoder_info isn't valid, check again
        if (!decoder_info.valid) {
            decoder_check_info(decoder_current, decoder_current_libidx);
        }
    }
    // If audio isn't initialized, init here
    if (audio_current == AUDIO_NONE) {
        audio_init(app_configuration->audio_backend, argc, argv);
    }

    applog_i("APP", "Decoder module: %s (%s requested)", decoder_definitions[decoder_current].name,
             app_configuration->decoder);
    if (audio_current == AUDIO_DECODER) {
        applog_i("APP", "Audio module: decoder implementation (%s requested)\n", app_configuration->audio_backend);
    } else if (audio_current >= AUDIO_FIRST) {
        applog_i("APP", "Audio module: %s (%s requested)", audio_definitions[audio_current].name,
                 app_configuration->audio_backend);
    }
}

void module_seterror(const char *error) {
    if (!error) {
        module_error[0] = '\0';
        return;
    }
    strncpy(module_error, error, 1023);
    module_error[1023] = '\0';
}

const char *module_geterror() {
    return module_error;
}

bool module_verify(const os_info_t *os_info, const MODULE_DEFINITION *def) {
    if (!def->liblen) return true;
#if FEATURE_CHECK_MODULE_OS_VERSION
    if (!os_info->version) return true;
    MODULE_OS_REQUIREMENT req = def->os_req;
    // Return false if system is too old
    if (req.min_inclusive && os_info->version < req.min_inclusive) return false;
    // Return false if system is too new
    if (req.max_exclusive && os_info->version >= req.max_exclusive) return false;
#endif
    return true;
}