#pragma once

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#ifdef RES_IMPL
INCBIN(default_cover, "../res/defcover.png");
INCBIN(spritesheet_ui_1x, "../res/spritesheet_ui@2x.png");
INCBIN(spritesheet_ui_2x, "../res/spritesheet_ui@2x.png");
INCBIN(spritesheet_ui_3x, "../res/spritesheet_ui@3x.png");
#if TARGET_DESKTOP || TARGET_RASPI
INCBIN(window_icon_32, "../res/moonlight_32.png");
#endif
#else
INCBIN_EXTERN(default_cover);
INCBIN_EXTERN(spritesheet_ui_1x);
INCBIN_EXTERN(spritesheet_ui_2x);
INCBIN_EXTERN(spritesheet_ui_3x);
#if TARGET_DESKTOP || TARGET_RASPI
INCBIN_EXTERN(window_icon_32);
#endif
#endif