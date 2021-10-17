#pragma once

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#ifdef RES_IMPL
INCBIN(default_cover, "../res/defcover.png");
INCBIN(logo_96, "../res/moonlight.png");
INCBIN(fav_indicator, "../res/fav_indicator.png");
#else
INCBIN_EXTERN(default_cover);
INCBIN_EXTERN(logo_96);
INCBIN_EXTERN(fav_indicator);

#endif