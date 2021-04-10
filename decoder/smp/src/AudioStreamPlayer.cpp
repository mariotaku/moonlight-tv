#include "AudioStreamPlayer.h"

#include <pbnjson.hpp>

using namespace SMP_DECODER_NS;
namespace pj = pbnjson;

AudioStreamPlayer::AudioStreamPlayer(AudioConfig &audioConfig) : AbsStreamPlayer()
{
}

AudioStreamPlayer::~AudioStreamPlayer()
{
}

void AudioStreamPlayer::submit(char *sampleData, int sampleLength)
{
    submitBuffer(sampleData, sampleLength, 0, 2);
}

std::string AudioStreamPlayer::makeLoadPayload(AudioConfig &audioConfig, uint64_t time)
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

    arg.put("mediaTransportType", "BUFFERSTREAM");

    pj::JValue audioSink = pj::Object();
    audioSink.put("type", "main_sound");
    avSink.put("audioSink", audioSink);
    codec.put("audio", "OPUS");
    pj::JValue opusInfo = pj::Object();
    opusInfo.put("channels", std::to_string(audioConfig.opusConfig.channelCount));
    opusInfo.put("sampleRate", static_cast<double>(audioConfig.opusConfig.sampleRate / 1000.f));
    contents.put("opusInfo", opusInfo);

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

    externalStreamingInfo.put("contents", contents);
    externalStreamingInfo.put("bufferingCtrInfo", bufferingCtrInfo);

    pj::JValue transmission = pj::JObject();
    transmission.put("contentsType", "WEBRTC");

    option.put("appId", app_id_);
    option.put("avSink", avSink);
    // option.put("queryPosition", false);
    option.put("externalStreamingInfo", externalStreamingInfo);
    option.put("lowDelayMode", true);
    option.put("transmission", transmission);
    option.put("needAudio", true);

    arg.put("option", option);
    args.append(arg);
    payload.put("args", args);

    return pbnjson::JGenerator::serialize(payload, pbnjson::JSchemaFragment("{}"));
}

void AudioStreamPlayer::SetMediaAudioData(const char *data)
{
    std::cout << "AudioStreamPlayer::SetMediaAudioData" << data << std::endl;
}

void AudioStreamPlayer::LoadCallback(int type, int64_t numValue, const char *strValue)
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
    case PF_EVENT_TYPE_STR_AUDIO_INFO:
        SetMediaAudioData(strValue);
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

void AudioStreamPlayer::LoadCallback(int type, int64_t numValue, const char *strValue, void *data)
{
    AudioStreamPlayer *player = static_cast<AudioStreamPlayer *>(data);
    player->LoadCallback(type, numValue, strValue);
}