#include "platform.h"

bool decoder_check_lgnc(PDECODER_INFO dinfo);

extern AUDIO_RENDERER_CALLBACKS audio_callbacks_lgnc;
extern DECODER_RENDERER_CALLBACKS decoder_callbacks_lgnc;

DECODER_SYMBOLS decoder_lgnc = {
    true,
    NULL,
    &decoder_check_lgnc,
    NULL,
    &audio_callbacks_lgnc,
    &decoder_callbacks_lgnc,
    NULL,
    NULL,
};