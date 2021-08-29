#pragma once

extern float ui_scale;

#if TARGET_WEBOS || TARGET_RASPI
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#elif TARGET_LGNC
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#else
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540
#endif

#define KEY_REPEAT_DURATION 300

#if TARGET_WEBOS
#define FONT_FAMILY "Museo Sans"
#define FONT_SIZE_DEFAULT 20
#else
#define FONT_FAMILY "Sans Serif"
#define FONT_SIZE_DEFAULT 18
#endif