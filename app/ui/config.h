#pragma once

#ifdef OS_WEBOS
#ifndef WEBOS_LEGACY
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define NK_UI_SCALE 3
#else
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define NK_UI_SCALE 2
#endif
#else
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576
#define NK_UI_SCALE 1.5
#endif