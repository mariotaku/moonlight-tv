#pragma once
#include <pthread.h>

#include "stream/api.h"

#define SDL_BUFFER_FRAMES 2

extern pthread_mutex_t mutex_ffsw;

extern RenderQueueSubmit render_queue_submit_ffmpeg;