#pragma once
#include <pthread.h>

#include "stream/module/api.h"

#define SDL_BUFFER_FRAMES 2

extern pthread_mutex_t mutex_ffsw;
extern int render_current_frame_ffmpeg, render_next_frame_ffmpeg;

extern RenderQueueSubmit render_queue_submit_ffmpeg;