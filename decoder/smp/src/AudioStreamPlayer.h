#pragma once

#include <Limelight.h>

extern "C" AUDIO_RENDERER_CALLBACKS audio_callbacks_smp;

namespace SMP_DECODER_NS
{
    class AudioStreamPlayer
    {
    public:
        static int setup(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags);
        static void start();
        static void submit(char *sampleData, int sampleLength);
        static void stop();
        static void cleanup();
    };
}
