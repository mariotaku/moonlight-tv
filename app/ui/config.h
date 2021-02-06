#pragma once

#if OS_WEBOS || TARGET_RASPI
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define NK_UI_SCALE 3
#else
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540
#define NK_UI_SCALE 1.5
#endif

#define KEY_REPEAT_DURATION 300
