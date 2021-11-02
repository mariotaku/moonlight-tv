#pragma once

#include "stream/module/api.h"

#define SDL_BUFFER_FRAMES 2

extern int render_current_frame_ffmpeg, render_next_frame_ffmpeg;

extern HOST_RENDER_CONTEXT *host_render_context_ffmpeg;