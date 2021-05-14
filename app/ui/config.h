#pragma once

float ui_scale;

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
#define NK_UI_SCALE ui_scale

#define KEY_REPEAT_DURATION 300

#if TARGET_WEBOS
#define FONT_FAMILY "Museo Sans"
#else
#define FONT_FAMILY "Sans Serif"
#endif