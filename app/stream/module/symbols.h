#pragma once

#include "stream/platform.h"

#if DECODER_FFMPEG_STATIC
extern const DECODER_SYMBOLS decoder_ffmpeg;
#define DECODER_SYMBOLS_FFMPEG &decoder_ffmpeg
#else
#define DECODER_SYMBOLS_FFMPEG NULL
#endif