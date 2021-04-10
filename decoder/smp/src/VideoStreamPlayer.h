#pragma once
#include "AbsStreamPlayer.h"

#include <memory>

#include <Limelight.h>

#include <StarfishMediaAPIs.h>

#ifdef USE_ACB
#include <Acb.h>
#endif

namespace SMP_DECODER_NS
{
    struct VideoConfig
    {
        int format;
        int width, height;
        int fps;
    };
    class VideoStreamPlayer : public AbsStreamPlayer
    {
    public:
        VideoStreamPlayer(VideoConfig &videoConfig);
        ~VideoStreamPlayer();
        int submit(PDECODE_UNIT decodeUnit);

    private:
        static void LoadCallback(int type, int64_t numValue, const char *strValue, void *data);

        void LoadCallback(int type, int64_t numValue, const char *strValue);
        void SetMediaVideoData(const char *data);
        std::string makeLoadPayload(VideoConfig &videoConfig, uint64_t time);

#ifdef USE_SDL_WEBOS
        const char *window_id_;
#endif
    };
}