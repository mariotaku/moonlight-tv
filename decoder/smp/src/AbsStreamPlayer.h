#pragma once

#include <memory>
#include <string>

#include <StarfishMediaAPIs.h>

#ifdef USE_ACB
#include <Acb.h>
#endif

namespace SMP_DECODER_NS
{
    enum PlayerState
    {
        UNINITIALIZED,
        UNLOADED,
        LOADED,
        PLAYING,
        EOS
    };

    class AbsStreamPlayer
    {
    public:
        explicit AbsStreamPlayer();
        ~AbsStreamPlayer();
        void sendEOS();

    protected:
        bool submitBuffer(const void *data, size_t size, uint64_t pts, int esData);
        PlayerState player_state_ = PlayerState::UNINITIALIZED;
        std::string app_id_;
        std::unique_ptr<StarfishMediaAPIs> starfish_media_apis_;
#ifdef USE_ACB
        std::unique_ptr<Acb> acb_client_;
        void AcbHandler(long acb_id, long task_id, long event_type, long app_state, long play_state, const char *reply);
#endif
    };
}