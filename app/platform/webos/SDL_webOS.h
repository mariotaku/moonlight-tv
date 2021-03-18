#pragma once

#include <SDL_types.h>
#include <SDL_rect.h>

#define SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK "SDL_WEBOS_ACCESS_POLICY_KEYS_BACK"
#define SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT "SDL_WEBOS_ACCESS_POLICY_KEYS_EXIT"
#define SDL_HINT_WEBOS_CURSOR_SLEEP_TIME "SDL_WEBOS_CURSOR_SLEEP_TIME"
#define SDL_HINT_WEBOS_REGISTER_APP "SDL_HINT_WEBOS_REGISTER_APP"

#define SDL_WEBOS_SCANCODE_CH_UP 480
#define SDL_WEBOS_SCANCODE_CH_DOWN 481
#define SDL_WEBOS_SCANCODE_BACK 482
#define SDL_WEBOS_SCANCODE_CURSOR_SHOW 484
#define SDL_WEBOS_SCANCODE_CURSOR_HIDE 485
#define SDL_WEBOS_SCANCODE_RED 486
#define SDL_WEBOS_SCANCODE_GREEN 487
#define SDL_WEBOS_SCANCODE_YELLOW 488
#define SDL_WEBOS_SCANCODE_BLUE 489
#define SDL_WEBOS_SCANCODE_EXIT 505

#define SDL_WEBOS_EXPORED_WINDOW_TYPE_VIDEO 0
#define SDL_WEBOS_EXPORED_WINDOW_TYPE_SUBTITLE 1
#define SDL_WEBOS_EXPORED_WINDOW_TYPE_TRANSPARENT 2
#define SDL_WEBOS_EXPORED_WINDOW_TYPE_OPAQUE 3

#if __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC SDL_bool SDL_webOSCursorVisibility(SDL_bool visible);

EXTERNC SDL_bool SDL_webOSGetPanelResolution(int *width, int *height);

EXTERNC SDL_bool SDL_webOSGetRefreshRate(int *rate);

EXTERNC const char *SDL_webOSCreateExportedWindow(int type);

EXTERNC SDL_bool SDL_webOSSetExportedWindow(const char *windowId, SDL_Rect *src, SDL_Rect *dst);

EXTERNC SDL_bool SDL_webOSExportedSetCropRegion(const char *windowId, SDL_Rect *org, SDL_Rect *src, SDL_Rect *dst);

EXTERNC SDL_bool SDL_webOSExportedSetProperty(const char *windowId, const char *name, const char *value);

EXTERNC void SDL_webOSDestroyExportedWindow(const char *windowId);

#undef EXTERNC