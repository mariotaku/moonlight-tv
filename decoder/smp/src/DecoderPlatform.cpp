#include "VideoStreamPlayer.h"

#include <cassert>
#include <iostream>

static bool smp_initialized = false;

using MoonlightStarfish::VideoStreamPlayer;
static VideoStreamPlayer *videoPlayer = nullptr;

extern "C" DECODER_RENDERER_CALLBACKS decoder_callbacks_smp;

extern "C" bool platform_init_smp(int argc, char *argv[])
{
    smp_initialized = true;
    return smp_initialized;
}

extern "C" bool platform_check_smp()
{
    return true;
}

extern "C" void platform_finalize_smp()
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

DECODER_RENDERER_CALLBACKS decoder_callbacks_smp = {
    .setup = _videoSetup,
    .start = _videoStart,
    .stop = _videoStop,
    .cleanup = _videoCleanup,
    .submitDecodeUnit = _videoSubmit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
