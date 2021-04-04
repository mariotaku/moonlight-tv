#include "AudioStreamPlayer.h"

using MoonlightStarfish::AudioStreamPlayer;

AUDIO_RENDERER_CALLBACKS audio_callbacks_smp = {
    .init = AudioStreamPlayer::setup,
    .start = AudioStreamPlayer::start,
    .stop = AudioStreamPlayer::stop,
    .cleanup = AudioStreamPlayer::cleanup,
    .decodeAndPlaySample = AudioStreamPlayer::submit,
    .capabilities = CAPABILITY_DIRECT_SUBMIT,
};
