#include "VideoStreamPlayer.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <pbnjson.hpp>

#ifdef USE_SDL_WEBOS
#include <SDL.h>
#endif

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

using SMP_DECODER_NS::VideoStreamPlayer;
namespace pj = pbnjson;

VideoStreamPlayer::VideoStreamPlayer(int videoFormat, int width, int height, int redrawRate)
    : player_state_(PlayerState::UNINITIALIZED)
{
    printf("[thread:%d] StarfishMediaAPIs::ctor\n", syscall(__NR_gettid));
    app_id_ = getenv("APPID");
    starfish_media_apis_.reset(new StarfishMediaAPIs());
#ifdef USE_ACB
    acb_client_.reset(new Acb());
    if (!acb_client_)
        return;
    auto acbHandler = std::bind(&VideoStreamPlayer::AcbHandler, this, acb_client_->getAcbId(),
                                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                std::placeholders::_4, std::placeholders::_5);
    if (!acb_client_->initialize(ACB::PlayerType::MSE, app_id_, acbHandler))
    {
        std::cout << "Acb::initialize() failed!" << std::endl;
        return;
    }
#endif

    starfish_media_apis_->notifyForeground();
#ifdef USE_SDL_WEBOS
    window_id_ = SDL_webOSCreateExportedWindow(0);
#endif
    player_state_ = PlayerState::UNLOADED;

    std::string payload = makeLoadPayload(videoFormat, width, height, redrawRate, 0);
    if (!starfish_media_apis_->Load(payload.c_str(), &LoadCallback, this))
    {
        std::cout << "StarfishMediaAPIs::Load() failed!" << std::endl;
        return;
    }
    player_state_ = PlayerState::LOADED;

#ifdef USE_ACB
    if (!acb_client_->setDisplayWindow(0, 0, width, height, true))
    {
        std::cout << "Acb::setDisplayWindow() failed!" << std::endl;
        return;
    }
#endif

#ifdef USE_SDL_WEBOS
    SDL_Rect src = {0, 0, width, height};
    SDL_Rect dst = {0, 0, 1920, 1080};
    SDL_webOSSetExportedWindow(window_id_, &src, &dst);
#endif
}

VideoStreamPlayer::~VideoStreamPlayer()
{
    std::cout << "VideoStreamPlayer::dtor" << std::endl;
#ifdef USE_ACB
    acb_client_->setState(ACB::AppState::FOREGROUND, ACB::PlayState::UNLOADED);
    acb_client_->finalize();
#endif

    starfish_media_apis_.reset(nullptr);
#ifdef USE_SDL_WEBOS
    SDL_webOSDestroyExportedWindow(window_id_);
#endif
}

void VideoStreamPlayer::start()
{
    std::cout << "VideoStreamPlayer::start" << std::endl;
}

int VideoStreamPlayer::submit(PDECODE_UNIT decodeUnit)
{
    if (player_state_ != LOADED && player_state_ != PLAYING)
    {
        std::cout << "Player not ready to feed" << std::endl;
        return DR_NEED_IDR;
    }
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
        if (player_state_ == LOADED)
        {
#ifdef USE_ACB
            if (!acb_client_->setState(ACB::AppState::FOREGROUND, ACB::PlayState::SEAMLESS_LOADED))
                std::cout << "Acb::setState(FOREGROUND, SEAMLESS_LOADED) failed!" << std::endl;
#endif
            player_state_ = PLAYING;
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
    player_state_ = PlayerState::UNLOADED;
    starfish_media_apis_->pushEOS();
}

std::string VideoStreamPlayer::makeLoadPayload(int videoFormat, int width, int height, int fps, uint64_t time)
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
        codec.put("video", "H265");
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
        printf("LoadCallback PF_EVENT_TYPE_STR_ERROR, numValue: %d, strValue: %p\n", numValue, strValue);
        break;
    case PF_EVENT_TYPE_INT_ERROR:
        printf("LoadCallback PF_EVENT_TYPE_INT_ERROR, numValue: %s, strValue: %p\n", numValue, strValue);
        break;
    case PF_EVENT_TYPE_STR_BUFFERFULL:
        printf("PF_EVENT_TYPE_STR_BUFFERFULL\n");
        break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
        std::cout << "PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED" << std::endl;
#ifdef USE_ACB
        acb_client_->setSinkType(ACB::StateSinkType::SINK_AUTO);
        acb_client_->setMediaId(starfish_media_apis_->getMediaID());
        acb_client_->setState(ACB::AppState::FOREGROUND, ACB::PlayState::LOADED);
#endif
        starfish_media_apis_->Play();
        break;
    case PF_EVENT_TYPE_STR_VIDEO_INFO:
        std::cout << "PF_EVENT_TYPE_STR_VIDEO_INFO" << std::endl;
        SetMediaVideoData(strValue);
        break;
    default:
        printf("LoadCallback type: 0x%02x, numValue: %d, strValue: %p\n", type, numValue, strValue);
        break;
    }
}

#ifdef USE_ACB
void VideoStreamPlayer::AcbHandler(long acb_id, long task_id, long event_type, long app_state, long play_state, const char *reply)
{
    printf("AcbHandler acbId = %ld, taskId = %ld, eventType = %ld, appState = %ld,playState = %ld, reply = %s EOL\n",
           acb_id, task_id, event_type, app_state, play_state, reply);
}
#endif

void VideoStreamPlayer::LoadCallback(int type, int64_t numValue, const char *strValue, void *data)
{
    VideoStreamPlayer *player = static_cast<VideoStreamPlayer *>(data);
    player->LoadCallback(type, numValue, strValue);
}