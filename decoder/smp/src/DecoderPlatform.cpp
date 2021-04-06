#include "VideoStreamPlayer.h"

#include <cassert>
#include <iostream>

#include "stream/platform.h"

#ifndef DECODER_PLATFORM_NAME
#error "DECODER_PLATFORM_NAME Not defined"
#endif

// Coming from https://stackoverflow.com/a/1489985/859190
#define DECODER_DECL_PASTER(x, y) x##_##y
#define DECODER_DECL_EVALUATOR(x, y) DECODER_DECL_PASTER(x, y)
#define DECODER_SYMBOL_NAME(name) DECODER_DECL_EVALUATOR(name, DECODER_PLATFORM_NAME)

#define platform_init DECODER_SYMBOL_NAME(platform_init)
#define platform_check DECODER_SYMBOL_NAME(platform_check)
#define platform_finalize DECODER_SYMBOL_NAME(platform_finalize)
#define decoder_callbacks DECODER_SYMBOL_NAME(decoder_callbacks)

static bool smp_initialized = false;

using SMP_DECODER_NS::VideoStreamPlayer;
static VideoStreamPlayer *videoPlayer = nullptr;

extern "C" DECODER_RENDERER_CALLBACKS decoder_callbacks;

extern "C" bool platform_init(int argc, char *argv[])
{
    smp_initialized = true;
    return smp_initialized;
}

extern "C" bool platform_check(PPLATFORM_INFO platform_info)
{
    platform_info->valid = true;
    platform_info->hevc = true;
    platform_info->hdr = PLATFORM_HDR_ALWAYS;
    platform_info->colorSpace = COLORSPACE_REC_709;
    platform_info->colorRange = COLOR_RANGE_FULL;
    return true;
}

extern "C" void platform_finalize()
{
}

static int _videoSetup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    assert(!videoPlayer);
    videoPlayer = new VideoStreamPlayer(videoFormat, width, height, redrawRate);
    return 0;
}

static void _videoStart()
{
    assert(videoPlayer);
    videoPlayer->start();
}

static int _videoSubmit(PDECODE_UNIT decodeUnit)
{
    assert(videoPlayer);
    return videoPlayer->submit(decodeUnit);
}

static void _videoStop()
{
    assert(videoPlayer);
    videoPlayer->stop();
}

static void _videoCleanup()
{
    assert(videoPlayer);
    delete videoPlayer;
    videoPlayer = nullptr;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks = {
    .setup = _videoSetup,
    .start = _videoStart,
    .stop = _videoStop,
    .cleanup = _videoCleanup,
    .submitDecodeUnit = _videoSubmit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
