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
        AVStreamPlayer();
        ~AVStreamPlayer();
        bool load();
        int submitVideo(PDECODE_UNIT decodeUnit);
        void submitAudio(char *sampleData, int sampleLength);
        void sendEOS();
        void setHdr(bool hdr);

        VideoConfig videoConfig;
        AudioConfig audioConfig;

    private:
        enum PlayerState
        {
            UNINITIALIZED,
            UNLOADED,
            LOADED,
            PLAYING,
            EOS,
        };

        std::string makeLoadPayload(VideoConfig &vidCfg, AudioConfig &audCfg, uint64_t time);
        std::string makeOpusHeader(OPUS_MULTISTREAM_CONFIGURATION &opusConfig);
        bool submitBuffer(const void *data, size_t size, uint64_t pts, int esData);

        void SetMediaAudioData(const char *data);
        void SetMediaVideoData(const char *data);
        void LoadCallback(int type, int64_t numValue, const char *strValue);

        static void LoadCallback(int type, int64_t numValue, const char *strValue, void *data);

        std::unique_ptr<StarfishMediaAPIs> starfish_media_apis_;
        std::string app_id_;
        PlayerState player_state_;
        char *video_buffer_;
        unsigned long long video_pts_;
        bool request_interrupt_;
        bool hdr_;
#ifdef USE_ACB
        void AcbHandler(long acb_id, long task_id, long event_type, long app_state, long play_state, const char *reply);
        std::unique_ptr<Acb> acb_client_;
#endif
#ifdef USE_SDL_WEBOS
        const char *window_id_;
#endif
    };
}