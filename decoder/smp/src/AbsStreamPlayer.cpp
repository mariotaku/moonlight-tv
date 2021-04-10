#include "AbsStreamPlayer.h"

#include <iostream>
#include <Limelight.h>

using SMP_DECODER_NS::AbsStreamPlayer;

AbsStreamPlayer::AbsStreamPlayer()
{
    app_id_ = getenv("APPID");
    starfish_media_apis_.reset(new StarfishMediaAPIs());
#ifdef USE_ACB
    acb_client_.reset(new Acb());
    if (!acb_client_)
        return;
    auto acbHandler = std::bind(&AbsStreamPlayer::AcbHandler, this, acb_client_->getAcbId(),
                                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                std::placeholders::_4, std::placeholders::_5);
    if (!acb_client_->initialize(ACB::PlayerType::MSE, app_id_, acbHandler))
    {
        std::cerr << "Acb::initialize() failed!" << std::endl;
        return;
    }
#endif

    starfish_media_apis_->notifyForeground();
}

AbsStreamPlayer::~AbsStreamPlayer()
{
#ifdef USE_ACB
    acb_client_->setState(ACB::AppState::FOREGROUND, ACB::PlayState::UNLOADED);
    acb_client_->finalize();
#endif

    starfish_media_apis_.reset(nullptr);
}

bool AbsStreamPlayer::submitBuffer(const void *data, size_t size, uint64_t pts, int esData)
{
    if (player_state_ == EOS)
    {
        // Player has marked as end of stream, ignore all data
        return DR_OK;
    }
    else if (player_state_ != LOADED && player_state_ != PLAYING)
    {
        std::cerr << "Player not ready to feed" << std::endl;
        return DR_NEED_IDR;
    }
    char payload[256];
    snprintf(payload, sizeof(payload), "{\"bufferAddr\":\"%x\",\"bufferSize\":%d,\"pts\":%llu,\"esData\":%d}",
             data, size, pts, esData);
    starfish_media_apis_->Feed(payload);
    if (player_state_ == LOADED)
    {
#ifdef USE_ACB
        if (!acb_client_->setState(ACB::AppState::FOREGROUND, ACB::PlayState::SEAMLESS_LOADED))
            std::cerr << "Acb::setState(FOREGROUND, SEAMLESS_LOADED) failed!" << std::endl;
#endif
        player_state_ = PLAYING;
    }
}

void AbsStreamPlayer::sendEOS()
{
    if (player_state_ != PLAYING)
        return;
    player_state_ = PlayerState::EOS;
    starfish_media_apis_->pushEOS();
}

#ifdef USE_ACB
void AbsStreamPlayer::AcbHandler(long acb_id, long task_id, long event_type, long app_state, long play_state, const char *reply)
{
    printf("AcbHandler acbId = %ld, taskId = %ld, eventType = %ld, appState = %ld,playState = %ld, reply = %s EOL\n",
           acb_id, task_id, event_type, app_state, play_state, reply);
}
#endif