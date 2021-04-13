#include "AbsStreamPlayer.h"
#include "AudioStreamPlayer.h"
#include "VideoStreamPlayer.h"

#include <cassert>
#include <iostream>

#include "stream/api.h"

#include <memory>
#include <cstring>

#define platform_init DECODER_SYMBOL_NAME(platform_init)
#define platform_check DECODER_SYMBOL_NAME(platform_check)
#define platform_finalize DECODER_SYMBOL_NAME(platform_finalize)
#define decoder_callbacks DECODER_SYMBOL_NAME(decoder_callbacks)
#define audio_callbacks DECODER_SYMBOL_NAME(audio_callbacks)

static bool smp_initialized = false;

using SMP_DECODER_NS::AudioConfig;
using SMP_DECODER_NS::AudioStreamPlayer;
using SMP_DECODER_NS::VideoConfig;
using SMP_DECODER_NS::VideoStreamPlayer;

static std::unique_ptr<AudioStreamPlayer> audioPlayer;
static std::unique_ptr<VideoStreamPlayer> videoPlayer;

static AudioConfig audioConfig;
static VideoConfig videoConfig;

extern "C" DECODER_RENDERER_CALLBACKS decoder_callbacks;
extern "C" AUDIO_RENDERER_CALLBACKS audio_callbacks;

extern "C" bool platform_init(int argc, char *argv[])
{
    smp_initialized = true;
    return smp_initialized;
}

extern "C" bool platform_check(PPLATFORM_INFO pinfo)
{
    pinfo->valid = true;
    pinfo->vrank = 40;
    pinfo->arank = 5;
    pinfo->vindependent = true;
    pinfo->aindependent = true;
    pinfo->hevc = true;
    pinfo->hdr = PLATFORM_HDR_ALWAYS;
    pinfo->colorSpace = COLORSPACE_REC_709;
    pinfo->colorRange = COLOR_RANGE_FULL;
    pinfo->maxBitrate = 60000;
    return true;
}

extern "C" void platform_finalize()
{
}

static int _videoSetup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    assert(!videoConfig.format);
    videoConfig.format = videoFormat;
    videoConfig.width = width;
    videoConfig.height = height;
    videoConfig.fps = redrawRate;
    videoPlayer.reset(new VideoStreamPlayer(videoConfig));
    return 0;
}

static int _audioSetup(int audioConfiguration, const POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags)
{
    assert(!audioConfig.type);
    audioConfig.type = audioConfiguration;
    memcpy(&audioConfig.opusConfig, opusConfig, sizeof(OPUS_MULTISTREAM_CONFIGURATION));
    audioPlayer.reset(new AudioStreamPlayer(audioConfig));
    return 0;
}

static int _videoSubmit(PDECODE_UNIT decodeUnit)
{
    assert(videoPlayer);
    return videoPlayer->submit(decodeUnit);
}

static void _audioSubmit(char *sampleData, int sampleLength)
{
    assert(audioPlayer);
    audioPlayer->submit(sampleData, sampleLength);
}

static void _audioStop()
{
    assert(audioPlayer);
    audioPlayer->sendEOS();
}
static void _videoStop()
{
    assert(videoPlayer);
    videoPlayer->sendEOS();
}

static void _videoCleanup()
{
    videoConfig.format = 0;
    videoPlayer.reset(nullptr);
}

static void _audioCleanup()
{
    audioConfig.type = 0;
    audioPlayer.reset(nullptr);
}

DECODER_RENDERER_CALLBACKS decoder_callbacks = {
    .setup = _videoSetup,
    .start = nullptr,
    .stop = _videoStop,
    .cleanup = _videoCleanup,
    .submitDecodeUnit = _videoSubmit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};

AUDIO_RENDERER_CALLBACKS audio_callbacks = {
    .init = _audioSetup,
    .start = nullptr,
    .stop = _audioStop,
    .cleanup = _audioCleanup,
    .decodeAndPlaySample = _audioSubmit,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
