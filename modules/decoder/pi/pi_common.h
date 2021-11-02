#pragma once
#include "stream/module/api.h"
#include "util/logging.h"

#include <ilclient.h>

// Define name to be a less common one, to avoid symbol conflicts
#define video_render video_render_pivid

extern COMPONENT_T *video_render;
extern logvprintf_fn module_logvprintf;