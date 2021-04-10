#pragma once

#include <memory>

#include <Limelight.h>

#include <StarfishMediaAPIs.h>

#ifdef USE_ACB
#include <Acb.h>
#endif

namespace SMP_DECODER_NS
{
    struct AudioConfig
    {
        int type;
        OPUS_MULTISTREAM_CONFIGURATION opusConfig;
    };
    struct VideoConfig
    {
        int format;
        int width, height;
        int fps;
    };
    class AVStreamPlayer
    {
    public:
        AVStreamPlayer(AudioConfig &audioConfig, VideoConfig &videoConfig);
        ~AVStreamPlayer();
        int submitVideo(PDECODE_UNIT decodeUnit);
        void submitAudio(char *sampleData, int sampleLength);
        void sendEOS();

    private:
        enum PlayerState
        {
            UNINITIALIZED,
            UNLOADED,
            LOADED,
            PLAYING,
            EOS
        };

        static void LoadCallback(int type, int64_t numValue, const char *strValue, void *data);

        void LoadCallback(int type, int64_t numValue, const char *strValue);
        void SetMediaAudioData(const char *data);
        void SetMediaVideoData(const char *data);
        PlayerState player_state_;
        std::string app_id_;
        std::string makeLoadPayload(AudioConfig &audioConfig, VideoConfig &videoConfig, uint64_t time);
        bool submitBuffer(const void *data, size_t size, uint64_t pts, int esData);
        std::unique_ptr<StarfishMediaAPIs> starfish_media_apis_;
#ifdef USE_ACB
        std::unique_ptr<Acb> acb_client_;
        void AcbHandler(long acb_id, long task_id, long event_type, long app_state, long play_state, const char *reply);
#endif
#ifdef USE_SDL_WEBOS
        const char *window_id_;
#endif
    };
}