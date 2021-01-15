#pragma once

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#ifdef RES_IMPL
INCBIN(notosans_regular, "../res/NotoSans-Regular.ttf");
INCBIN(default_cover, "../res/defcover.png");
#else
INCBIN_EXTERN(notosans_regular);
INCBIN_EXTERN(default_cover);
#endif