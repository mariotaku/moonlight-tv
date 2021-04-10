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
    struct AudioConfig
    {
        int type;
        OPUS_MULTISTREAM_CONFIGURATION opusConfig;
    };
    class AudioStreamPlayer : public AbsStreamPlayer
    {
    public:
        AudioStreamPlayer(AudioConfig &audioConfig);
        ~AudioStreamPlayer();
        void submit(char *sampleData, int sampleLength);

    private:
        static void LoadCallback(int type, int64_t numValue, const char *strValue, void *data);

        void LoadCallback(int type, int64_t numValue, const char *strValue);
        void SetMediaAudioData(const char *data);
        std::string makeLoadPayload(AudioConfig &config, uint64_t time);
    };
}