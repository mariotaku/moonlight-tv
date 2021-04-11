#pragma once

typedef void (*PresenterEnterFullScreen)(void);
typedef void (*PresenterEnterOverlay)(int x, int y, int w, int h);

typedef bool (*RenderSetup)(PSTREAM_CONFIGURATION conf);
typedef void (*RenderCleanup)();

typedef struct _VIDEO_PRESENTER_CALLBACKS
{
    PresenterEnterFullScreen enterFullScreen;
    PresenterEnterOverlay enterOverlay;
} VIDEO_PRESENTER_CALLBACKS, *PVIDEO_PRESENTER_CALLBACKS;

typedef struct _VIDEO_RENDER_CALLBACKS
{
    RenderSetup renderSetup;
    RenderCleanup renderCleanup;
} VIDEO_RENDER_CALLBACKS, *PVIDEO_RENDER_CALLBACKS;