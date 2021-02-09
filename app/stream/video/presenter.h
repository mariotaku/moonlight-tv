#pragma once

typedef void(*PresenterEnterFullScreen)(void);
typedef void(*PresenterEnterOverlay)(int x, int y, int w, int h);

typedef struct _VIDEO_PRESENTER_CALLBACKS {
    PresenterEnterFullScreen enterFullScreen;
    PresenterEnterOverlay enterOverlay;
} VIDEO_PRESENTER_CALLBACKS, *PVIDEO_PRESENTER_CALLBACKS;
