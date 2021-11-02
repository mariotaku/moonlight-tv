#include "app.h"
#include "stream/platform.h"
#include "util/logging.h"

HOST_CONTEXT module_host_context;

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