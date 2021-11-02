#include "ffmpeg_symbols.h"

bool decoder_check_ffmpeg(PDECODER_INFO info) {
    info->valid = true;
    info->accelerated = false;
    info->audio = false;
    info->hasRenderer = true;
    return true;
}