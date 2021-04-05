#pragma once

#include <memory>

#include <Limelight.h>

#include <StarfishMediaAPIs.h>

#ifdef USE_ACB
#include <Acb.h>
#endif

namespace SMP_DECODER_NS
{
    class VideoStreamPlayer
    {
    public:
        VideoStreamPlayer(int videoFormat, int width, int height, int redrawRate);
        ~VideoStreamPlayer();
        void start();
        int submit(PDECODE_UNIT decodeUnit);
        void stop();

    private:
        enum PlayerState
        {
            UNINITIALIZED,
            UNLOADED,
            LOADED,
            PLAYING
        };

        static void LoadCallback(int type, int64_t numValue, const char *strValue, void *data);

        void LoadCallback(int type, int64_t numValue, const char *strValue);
        void SetMediaVideoData(const char *data);
        PlayerState player_state_;
        std::string app_id_;
        std::string makeLoadPayload(int videoFormat, int width, int height, int fps, uint64_t time);
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