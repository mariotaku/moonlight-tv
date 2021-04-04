#pragma once

#include <memory>

#include <StarfishMediaAPIs.h>
#include <Limelight.h>

namespace MoonlightStarfish
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
        std::string makeLoadPayload(int videoFormat, int width, int height, int fps, uint64_t time, const char *windowId);
        std::unique_ptr<StarfishMediaAPIs> starfish_media_apis_;
        const char *window_id_;
    };
}