#pragma once

#include <Limelight.h>

extern struct VIDEO_STATS vdec_summary_stats;
extern struct VIDEO_INFO vdec_stream_info;
extern struct VIDEO_STATS audio_summary_stats;

DECODER_RENDERER_CALLBACKS decoder_render_callbacks_delegate(PDECODER_RENDERER_CALLBACKS cb);