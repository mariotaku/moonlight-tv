#pragma once

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#ifdef RES_IMPL
INCBIN(notosans_regular, "../res/NotoSans-Regular.ttf");
INCBIN(default_cover, "../res/defcover.png");
INCBIN(spritesheet_ui_1x, "../res/spritesheet_ui@2x.png");
INCBIN(spritesheet_ui_2x, "../res/spritesheet_ui@2x.png");
INCBIN(spritesheet_ui_3x, "../res/spritesheet_ui@3x.png");
#if TARGET_DESKTOP || TARGET_RASPI
INCBIN(window_icon_32, "../res/moonlight_32.png");
#endif
#if HAVE_FFMPEG
#if HAVE_GL2
INCBIN(vertex_source, "../res/shaders/vertex_source_gl2.glsl");
INCBIN(fragment_source, "../res/shaders/fragment_source_gl2.glsl");
#elif HAVE_GLES2
INCBIN(vertex_source, "../res/shaders/vertex_source_gles2.glsl");
INCBIN(fragment_source, "../res/shaders/fragment_source_gles2.glsl");
#else
#error "No GL version found"
#endif
#endif
#else
INCBIN_EXTERN(notosans_regular);
INCBIN_EXTERN(default_cover);
INCBIN_EXTERN(spritesheet_ui_1x);
INCBIN_EXTERN(spritesheet_ui_2x);
INCBIN_EXTERN(spritesheet_ui_3x);
#if TARGET_DESKTOP || TARGET_RASPI
INCBIN_EXTERN(window_icon_32);
#endif
#if HAVE_FFMPEG
INCBIN_EXTERN(vertex_source);
INCBIN_EXTERN(fragment_source);
#endif
#endif