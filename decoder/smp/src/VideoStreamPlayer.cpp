#include "VideoStreamPlayer.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <glib.h>
#include <pbnjson.hpp>

#include "SDL_webOS.h"

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

using MoonlightStarfish::VideoStreamPlayer;
namespace pj = pbnjson;

static void StarfishDirectMediaPlayerLoadCallback(gint type, gint64 numValue, const gchar *strValue, void *data);

VideoStreamPlayer::VideoStreamPlayer(int videoFormat, int width, int height, int redrawRate)
{
    printf("[thread:%d] StarfishMediaAPIs::ctor\n", syscall(__NR_gettid));
    starfish_media_apis_.reset(new StarfishMediaAPIs());
    starfish_media_apis_->notifyForeground();
    window_id_ = SDL_webOSCreateExportedWindow(0);

    std::string payload = makeLoadPayload(videoFormat, width, height, redrawRate, 0, window_id_);
    if (!starfish_media_apis_->Load(payload.c_str(), StarfishDirectMediaPlayerLoadCallback, starfish_media_apis_.get()))
    {
        std::cout << "Load() failed!" << std::endl;
        return;
    }

    SDL_Rect src = {0, 0, width, height};
    SDL_Rect dst = {0, 0, 1920, 1080};
    SDL_webOSSetExportedWindow(window_id_, &src, &dst);
}

VideoStreamPlayer::~VideoStreamPlayer()
{
    std::cout << "VideoStreamPlayer::dtor" << std::endl;
    starfish_media_apis_.reset(nullptr);
    SDL_webOSDestroyExportedWindow(window_id_);
}

void VideoStreamPlayer::start()
{
    std::cout << "VideoStreamPlayer::start" << std::endl;
}

int VideoStreamPlayer::submit(PDECODE_UNIT decodeUnit)
{
    unsigned long long ms = decodeUnit->presentationTimeMs;
    unsigned long long pts = ms * 1000000ULL;
    if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
    {
        char payload[256];
        for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
        {
            snprintf(payload, sizeof(payload), "{\"bufferAddr\":\"%x\",\"bufferSize\":%d,\"pts\":%llu,\"esData\":1}",
                     entry->data, entry->length, pts);
            starfish_media_apis_->Feed(payload);
        }
        return DR_OK;
    }
    else
    {
        fprintf(stderr, "Video decode buffer too small, skip this frame");
        return DR_NEED_IDR;
    }
}

void VideoStreamPlayer::stop()
{
    std::cout << "VideoStreamPlayer::stop" << std::endl;
}

std::string VideoStreamPlayer::makeLoadPayload(int videoFormat, int width, int height, int fps, uint64_t time, const char *windowId)
{
    pj::JValue payload = pj::Object();
    pj::JValue args = pj::Array();
    pj::JValue arg = pj::Object();
    pj::JValue option = pj::Object();
    pj::JValue contents = pj::Object();
    pj::JValue bufferingCtrInfo = pj::Object();
    pj::JValue externalStreamingInfo = pj::Object();
    pj::JValue codec = pj::Object();
    pj::JValue avSink = pj::Object();
    pj::JValue videoSink = pj::Object();
    videoSink.put("type", "main_video");

    avSink.put("videoSink", videoSink);

    arg.put("mediaTransportType", "BUFFERSTREAM");
    pj::JValue adaptiveStreaming = pj::Object();
    adaptiveStreaming.put("maxWidth", width);
    adaptiveStreaming.put("maxHeight", height);
    adaptiveStreaming.put("maxFrameRate", fps);

    if (videoFormat & VIDEO_FORMAT_MASK_H264)
    {
        codec.put("video", "H264");
    }
    else if (videoFormat & VIDEO_FORMAT_MASK_H265)
    {
        codec.put("video", "HEVC");
    }

    contents.put("codec", codec);

    pj::JValue esInfo = pj::Object();
    esInfo.put("ptsToDecode", static_cast<int64_t>(time));
    esInfo.put("seperatedPTS", true);
    contents.put("esInfo", esInfo);

    contents.put("format", "RAW");
    // contents.put("provider", "Chrome");

    bufferingCtrInfo.put("bufferMaxLevel", 0);
    bufferingCtrInfo.put("bufferMinLevel", 0);
    bufferingCtrInfo.put("preBufferByte", 0);
    bufferingCtrInfo.put("qBufferLevelAudio", 0);
    bufferingCtrInfo.put("qBufferLevelVideo", 0);

    pj::JValue srcBufferLevelAudio = pj::Object();
    srcBufferLevelAudio.put("maximum", 2097152);
    srcBufferLevelAudio.put("minimum", 1024);
    bufferingCtrInfo.put("srcBufferLevelAudio", srcBufferLevelAudio);

    pj::JValue srcBufferLevelVideo = pj::Object();
    srcBufferLevelVideo.put("maximum", 8388608);
    srcBufferLevelVideo.put("minimum", 1);
    bufferingCtrInfo.put("srcBufferLevelVideo", srcBufferLevelVideo);

    externalStreamingInfo.put("contents", contents);
    // externalStreamingInfo.put("restartStreaming", false);
    // externalStreamingInfo.put("streamQualityInfo", true);
    // externalStreamingInfo.put("streamQualityInfoCorruptedFrame", true);
    // externalStreamingInfo.put("streamQualityInfoNonFlushable", true);
    externalStreamingInfo.put("bufferingCtrInfo", bufferingCtrInfo);

    pj::JValue transmission = pj::JObject();
    transmission.put("contentsType", "WEBRTC");

    option.put("adaptiveStreaming", adaptiveStreaming);
    option.put("appId", getenv("APPID"));
    option.put("avSink", avSink);
    // option.put("queryPosition", false);
    option.put("externalStreamingInfo", externalStreamingInfo);
    option.put("lowDelayMode", true);
    option.put("transmission", transmission);
    option.put("needAudio", false);
    if (windowId)
    {
        option.put("windowId", windowId);
    }

    arg.put("option", option);
    args.append(arg);
    payload.put("args", args);

    return pbnjson::JGenerator::serialize(payload, pbnjson::JSchemaFragment("{}"));
}

static void StarfishDirectMediaPlayerLoadCallback(gint type, gint64 numValue, const gchar *strValue, void *data)
{
    switch (type)
    {
    case 0:
        break;
    case PF_EVENT_TYPE_STR_ERROR:
        printf("LoadCallback PF_EVENT_TYPE_STR_ERROR, numValue: %d, strValue: %p\n", numValue, strValue);
        break;
    case PF_EVENT_TYPE_INT_ERROR:
        printf("LoadCallback PF_EVENT_TYPE_INT_ERROR, numValue: %s, strValue: %p\n", numValue, strValue);
        break;
    case PF_EVENT_TYPE_STR_BUFFERFULL:
        printf("PF_EVENT_TYPE_STR_BUFFERFULL\n");
        break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
        static_cast<StarfishMediaAPIs *>(data)->Play();
        break;
    default:
        printf("LoadCallback type: 0x%02x, numValue: %d, strValue: %p\n", type, numValue, strValue);
        break;
    }
}