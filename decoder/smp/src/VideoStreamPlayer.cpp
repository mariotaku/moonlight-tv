#include "VideoStreamPlayer.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <stdlib.h>

#include <pbnjson.hpp>

#ifdef USE_SDL_WEBOS
#include <SDL.h>
#endif

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

using SMP_DECODER_NS::VideoStreamPlayer;
namespace pj = pbnjson;

VideoStreamPlayer::VideoStreamPlayer(VideoConfig &videoConfig) : AbsStreamPlayer()
{
#ifdef USE_SDL_WEBOS
    window_id_ = SDL_webOSCreateExportedWindow(0);
#endif
    player_state_ = PlayerState::UNLOADED;

    std::string payload = makeLoadPayload(videoConfig, 0);
    if (!starfish_media_apis_->Load(payload.c_str(), &LoadCallback, this))
    {
        std::cerr << "StarfishMediaAPIs::Load() failed!" << std::endl;
        return;
    }
    player_state_ = PlayerState::LOADED;

#ifdef USE_ACB
    if (!acb_client_->setDisplayWindow(0, 0, videoConfig.width, videoConfig.height, true))
    {
        std::cerr << "Acb::setDisplayWindow() failed!" << std::endl;
        return;
    }
#endif

#ifdef USE_SDL_WEBOS
    SDL_Rect src = {0, 0, videoConfig.width, videoConfig.height};
    SDL_Rect dst = {0, 0, 1920, 1080};
    SDL_webOSSetExportedWindow(window_id_, &src, &dst);
#endif
}

VideoStreamPlayer::~VideoStreamPlayer()
{
#ifdef USE_SDL_WEBOS
    SDL_webOSDestroyExportedWindow(window_id_);
#endif
}

int VideoStreamPlayer::submit(PDECODE_UNIT decodeUnit)
{
    unsigned long long ms = decodeUnit->presentationTimeMs;
    unsigned long long pts = ms * 1000000ULL;
    if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
    {
        for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
        {
            submitBuffer(entry->data, entry->length, pts, 1);
        }
        return DR_OK;
    }
    else
    {
        fprintf(stderr, "Video decode buffer too small, skip this frame");
        return DR_NEED_IDR;
    }
}

std::string VideoStreamPlayer::makeLoadPayload(VideoConfig &videoConfig, uint64_t time)
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
    adaptiveStreaming.put("maxWidth", videoConfig.width);
    adaptiveStreaming.put("maxHeight", videoConfig.height);
    adaptiveStreaming.put("maxFrameRate", videoConfig.fps);

    if (videoConfig.format & VIDEO_FORMAT_MASK_H264)
        codec.put("video", "H264");
    else if (videoConfig.format & VIDEO_FORMAT_MASK_H265)
        codec.put("video", "H265");

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
    srcBufferLevelAudio.put("maximum", 10);
    srcBufferLevelAudio.put("minimum", 1);
    bufferingCtrInfo.put("srcBufferLevelAudio", srcBufferLevelAudio);

    pj::JValue srcBufferLevelVideo = pj::Object();
    srcBufferLevelVideo.put("maximum", DECODER_BUFFER_SIZE);
    srcBufferLevelVideo.put("minimum", 1);
    bufferingCtrInfo.put("srcBufferLevelVideo", srcBufferLevelVideo);

    externalStreamingInfo.put("contents", contents);
    externalStreamingInfo.put("bufferingCtrInfo", bufferingCtrInfo);

    pj::JValue transmission = pj::JObject();
    transmission.put("contentsType", "WEBRTC");

    option.put("adaptiveStreaming", adaptiveStreaming);
    option.put("appId", app_id_);
    option.put("avSink", avSink);
    // option.put("queryPosition", false);
    option.put("externalStreamingInfo", externalStreamingInfo);
    option.put("lowDelayMode", true);
    option.put("transmission", transmission);
    option.put("needAudio", false);
#ifdef USE_SDL_WEBOS
    option.put("windowId", window_id_);
#endif

    arg.put("option", option);
    args.append(arg);
    payload.put("args", args);

    return pbnjson::JGenerator::serialize(payload, pbnjson::JSchemaFragment("{}"));
}

void VideoStreamPlayer::SetMediaVideoData(const char *data)
{
    std::cout << "VideoStreamPlayer::SetMediaVideoData" << data << std::endl;
#ifdef USE_ACB
    acb_client_->setMediaVideoData(data);
#endif
}

void VideoStreamPlayer::LoadCallback(int type, int64_t numValue, const char *strValue)
{
    switch (type)
    {
    case 0:
        break;
    case PF_EVENT_TYPE_STR_ERROR:
        fprintf(stderr, "LoadCallback PF_EVENT_TYPE_STR_ERROR, numValue: %d, strValue: %p\n", numValue, strValue);
        break;
    case PF_EVENT_TYPE_INT_ERROR:
        fprintf(stderr, "LoadCallback PF_EVENT_TYPE_INT_ERROR, numValue: %s, strValue: %p\n", numValue, strValue);
        break;
    case PF_EVENT_TYPE_STR_BUFFERFULL:
        fprintf(stderr, "LoadCallback PF_EVENT_TYPE_STR_BUFFERFULL\n");
        break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
#ifdef USE_ACB
        acb_client_->setSinkType(ACB::StateSinkType::SINK_AUTO);
        acb_client_->setMediaId(starfish_media_apis_->getMediaID());
        acb_client_->setState(ACB::AppState::FOREGROUND, ACB::PlayState::LOADED);
#endif
        starfish_media_apis_->Play();
        break;
    case PF_EVENT_TYPE_STR_VIDEO_INFO:
        SetMediaVideoData(strValue);
        break;
    case PF_EVENT_TYPE_DROPPED_FRAME:
        fprintf(stderr, "LoadCallback PF_EVENT_TYPE_DROPPED_FRAME\n");
        break;
    case PF_EVENT_TYPE_INT_SVP_VDEC_READY:
        // fprintf(stderr, "LoadCallback PF_EVENT_TYPE_INT_SVP_VDEC_READY, numValue: %d, strValue: %p\n", numValue, strValue);
        break;
    default:
        fprintf(stderr, "LoadCallback unhandled 0x%02x\n", type);
        break;
    }
}

void VideoStreamPlayer::LoadCallback(int type, int64_t numValue, const char *strValue, void *data)
{
    VideoStreamPlayer *player = static_cast<VideoStreamPlayer *>(data);
    player->LoadCallback(type, numValue, strValue);
}