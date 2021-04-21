#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <Limelight.h>

#define DISPLAY_ROTATE_MASK 24
#define DISPLAY_ROTATE_90 8
#define DISPLAY_ROTATE_180 16
#define DISPLAY_ROTATE_270 24

#define DECODER_HDR_NONE 0
#define DECODER_HDR_SUPPORTED 1
#define DECODER_HDR_ALWAYS 2

// Corresponding to GS_OUT_OF_MEMORY
#define ERROR_OUT_OF_MEMORY -2
#define ERROR_UNKNOWN_CODEC 10
#define ERROR_DECODER_OPEN_FAILED 11
#define ERROR_DECODER_CLOSE_FAILED 12
#define ERROR_AUDIO_OPEN_FAILED 21
#define ERROR_AUDIO_CLOSE_FAILED 22
#define ERROR_AUDIO_OPUS_INIT_FAILED 23

#ifdef PLUGIN_SYMBOL_SUFFIX
// Coming from https://stackoverflow.com/a/1489985/859190
#define SYMBOL_DECL_PASTER(x, y) x##_##y
#define SYMBOL_DECL_EVALUATOR(x, y) SYMBOL_DECL_PASTER(x, y)
#define PLUGIN_SYMBOL_NAME(name) SYMBOL_DECL_EVALUATOR(name, PLUGIN_SYMBOL_SUFFIX)
#endif

#ifndef DECODER_EXPORTED
#define DECODER_EXPORTED __attribute__((visibility("default")))
#endif

typedef struct HOST_CONTEXT
{
    void (*logvprintf)(const char *, const char *, const char *, va_list);
} HOST_CONTEXT, *PHOST_CONTEXT;

typedef struct DECODER_INFO
{
    /* Decoder passes the check */
    bool valid;
    /* Decoder supports hardware acceleration */
    bool accelerated;
    /* Decoder has built-in audio feature */
    bool audio;
    /* Decoder supports HEVC video stream */
    bool hevc;
    /* Decoder supports HDR */
    int hdr;
    int colorSpace;
    int colorRange;
    int maxBitrate;
} * PDECODER_INFO, DECODER_INFO;

typedef struct AUDIO_INFO
{
    bool valid;
    int configuration;
} * PAUDIO_INFO, AUDIO_INFO;

typedef void (*PresenterEnterFullScreen)(void);
typedef void (*PresenterEnterOverlay)(int x, int y, int w, int h);

typedef bool (*RenderQueueSubmit)(void *);
typedef bool (*RenderSetup)(PSTREAM_CONFIGURATION conf, RenderQueueSubmit queueSubmit);
typedef bool (*RenderSubmit)(void *);
typedef bool (*RenderDraw)();
typedef void (*RenderCleanup)();

typedef struct _VIDEO_PRESENTER_CALLBACKS
{
    PresenterEnterFullScreen enterFullScreen;
    PresenterEnterOverlay enterOverlay;
} VIDEO_PRESENTER_CALLBACKS, *PVIDEO_PRESENTER_CALLBACKS;

typedef struct _VIDEO_RENDER_CALLBACKS
{
    RenderSetup renderSetup;
    RenderSubmit renderSubmit;
    RenderDraw renderDraw;
    RenderCleanup renderCleanup;
} VIDEO_RENDER_CALLBACKS, *PVIDEO_RENDER_CALLBACKS;