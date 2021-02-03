#pragma once

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#ifdef RES_IMPL
INCBIN(notosans_regular, "../res/NotoSans-Regular.ttf");
INCBIN(default_cover, "../res/defcover.png");
INCBIN(spritesheet_ui_2x, "../res/spritesheet_ui@2x.png");
INCBIN(spritesheet_ui_3x, "../res/spritesheet_ui@3x.png");
#ifdef TARGET_DESKTOP
INCBIN(window_icon_32, "../res/moonlight_32.png");
INCBIN(vertex_source, "../res/shaders/vertex_source_gl2.glsl");
INCBIN(fragment_source, "../res/shaders/fragment_source_gl2.glsl");
#endif
#else
INCBIN_EXTERN(notosans_regular);
INCBIN_EXTERN(default_cover);
INCBIN_EXTERN(spritesheet_ui_2x);
INCBIN_EXTERN(spritesheet_ui_3x);
#ifdef TARGET_DESKTOP
INCBIN_EXTERN(window_icon_32);
INCBIN_EXTERN(vertex_source);
INCBIN_EXTERN(fragment_source);
#endif
#endif