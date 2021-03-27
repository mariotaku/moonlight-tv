#pragma once

unsigned int ui_scale;

#if OS_WEBOS || TARGET_RASPI
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#else
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540
#endif
#define NK_UI_SCALE ui_scale

#define KEY_REPEAT_DURATION 300