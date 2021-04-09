#pragma once

#include <Limelight.h>

namespace SMP_DECODER_NS
{
    class AudioStreamPlayer
    {
    public:
        AudioStreamPlayer(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags);
        ~AudioStreamPlayer();
        void start();
        void submit(char *sampleData, int sampleLength);
        void stop();
    };
}
