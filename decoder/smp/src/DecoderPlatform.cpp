#include "AVStreamPlayer.h"

#include <cassert>
#include <iostream>

#include "stream/platform.h"

#include <memory>
#include <cstring>

#define platform_init DECODER_SYMBOL_NAME(platform_init)
#define platform_check DECODER_SYMBOL_NAME(platform_check)
#define platform_finalize DECODER_SYMBOL_NAME(platform_finalize)
#define decoder_callbacks DECODER_SYMBOL_NAME(decoder_callbacks)
#define audio_callbacks DECODER_SYMBOL_NAME(audio_callbacks)

static bool smp_initialized = false;

using SMP_DECODER_NS::AudioConfig;
using SMP_DECODER_NS::AVStreamPlayer;
using SMP_DECODER_NS::VideoConfig;

static std::unique_ptr<AVStreamPlayer> streamPlayer;

static AudioConfig audioConfig;
static VideoConfig videoConfig;

extern "C" DECODER_RENDERER_CALLBACKS decoder_callbacks;
extern "C" AUDIO_RENDERER_CALLBACKS audio_callbacks;

extern "C" bool platform_init(int argc, char *argv[])
{
    smp_initialized = true;
    return smp_initialized;
}

extern "C" bool platform_check(PPLATFORM_INFO platform_info)
{
    platform_info->valid = true;
    platform_info->hwaccel = true;
    platform_info->audio = true;
    platform_info->hevc = true;
    platform_info->hdr = PLATFORM_HDR_ALWAYS;
    platform_info->colorSpace = COLORSPACE_REC_709;
    platform_info->colorRange = COLOR_RANGE_FULL;
    platform_info->maxBitrate = 60000;
    return true;
}

extern "C" void platform_finalize()
{
}

static int _initPlayerWhenReady()
{
    if (!audioConfig.type || !videoConfig.format)
        return 0;
    streamPlayer.reset(new AVStreamPlayer(audioConfig, videoConfig));
    return 0;
}

static void _destroyPlayer()
{
    streamPlayer.reset(nullptr);
}

static int _videoSetup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    assert(!videoConfig.format);
    videoConfig.format = videoFormat;
    videoConfig.width = width;
    videoConfig.height = height;
    videoConfig.fps = redrawRate;
    return _initPlayerWhenReady();
}

static int _audioSetup(int audioConfiguration, const POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
    assert(!audioConfig.type);
    audioConfig.type = audioConfiguration;
    memcpy(&audioConfig.opusConfig, opusConfig, sizeof(OPUS_MULTISTREAM_CONFIGURATION));
    return _initPlayerWhenReady();
}

static int _videoSubmit(PDECODE_UNIT decodeUnit)
{
    if (!streamPlayer)
        return DR_OK;
    return streamPlayer->submitVideo(decodeUnit);
}

static void _audioSubmit(char *sampleData, int sampleLength)
{
    if (!streamPlayer)
        return;
    streamPlayer->submitAudio(sampleData, sampleLength);
}

static void _avStop()
{
    if (!streamPlayer)
        return;
    streamPlayer->sendEOS();
}

static void _videoCleanup()
{
    videoConfig.format = 0;
    _destroyPlayer();
}

static void _audioCleanup()
{
    audioConfig.type = 0;
    _destroyPlayer();
}

DECODER_RENDERER_CALLBACKS decoder_callbacks = {
    .setup = _videoSetup,
    .start = nullptr,
    .stop = _avStop,
    .cleanup = _videoCleanup,
    .submitDecodeUnit = _videoSubmit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};

AUDIO_RENDERER_CALLBACKS audio_callbacks = {
    .init = _audioSetup,
    .start = nullptr,
    .stop = _avStop,
    .cleanup = _audioCleanup,
    .decodeAndPlaySample = _audioSubmit,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
