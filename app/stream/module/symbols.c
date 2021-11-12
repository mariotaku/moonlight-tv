//
// Created by Mariotaku on 2021/09/12.
//

#include "symbols.h"

#if DECODER_FFMPEG_STATIC

#include "decoder/ffmpeg/ffmpeg_symbols.h"

const DECODER_SYMBOLS decoder_ffmpeg = {
        .valid = true,
        .init = decoder_init_ffmpeg,
        .check = decoder_check_ffmpeg,
        .vdec = &decoder_callbacks_ffmpeg,
        .rend = &render_callbacks_ffmpeg,
};
#endif