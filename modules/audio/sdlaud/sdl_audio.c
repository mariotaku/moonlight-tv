#include "module/api.h"
#include "module/logging.h"
#include "ringbuf.h"

#include <SDL.h>

#include <Limelight.h>
#include <opus_multistream.h>

logvprintf_fn MODULE_LOGVPRINTF;

static SDL_AudioDeviceID dev = 0;
static OpusMSDecoder *decoder = NULL;
static sdlaud_ringbuf *ringbuf = NULL;
static short *pcmBuffer = NULL;
static unsigned char *readbuf = NULL;
static size_t readbuf_size = 0;
static int channelCount, samplesPerFrame, sampleRate;

static void sdlaud_callback(void *userdata, Uint8 *stream, int len);

static int setup(int audioConfiguration, POPUS_MULTISTREAM_CONFIGURATION opusConfig, void *context, int arFlags) {
    int rc;
    samplesPerFrame = opusConfig->samplesPerFrame;
    channelCount = opusConfig->channelCount;
    sampleRate = opusConfig->sampleRate;

    decoder = opus_multistream_decoder_create(sampleRate, channelCount, opusConfig->streams, opusConfig->coupledStreams,
                                              opusConfig->mapping, &rc);

    size_t bufSize = sizeof(short) * channelCount * samplesPerFrame;
    // This ring buffer stores roughly 80ms of audio
    ringbuf = sdlaud_ringbuf_new(bufSize * 16);
    pcmBuffer = malloc(bufSize);

    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.callback = sdlaud_callback;
    want.freq = sampleRate;
    want.format = AUDIO_S16LSB;
    want.channels = channelCount;
    want.samples = samplesPerFrame;

    if (SDL_OpenAudio(&want, &have)) {
        applog_e("SDLAud", "Failed to open audio: %s", SDL_GetError());
        return -1;
    } else {
        if (have.format != want.format) // we let this one thing change.
            applog_w("SDLAud", "We didn't get requested audio format.");
        SDL_PauseAudio(0); // start audio playing.
    }

    return 0;
}

static void cleanup() {
    if (decoder != NULL) {
        opus_multistream_decoder_destroy(decoder);
        decoder = NULL;
    }
    if (ringbuf != NULL) {
        sdlaud_ringbuf_delete(ringbuf);
        ringbuf = NULL;
    }
    if (readbuf != NULL) {
        SDL_free(readbuf);
        readbuf_size = 0;
        readbuf = NULL;
    }
    if (pcmBuffer != NULL) {
        SDL_free(pcmBuffer);
        pcmBuffer = NULL;
    }
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

static void queue(char *data, int length) {
    int decodeLen = opus_multistream_decode(decoder, (unsigned char *) data, length, pcmBuffer, samplesPerFrame, 0);
    if (decodeLen > 0) {
        size_t write_size = sdlaud_ringbuf_write(ringbuf, (unsigned char *) pcmBuffer,
                                                 decodeLen * channelCount * sizeof(short));
        if (!write_size) {
            applog_w("SDLAud", "ring buffer overflow, clean the whole buffer");
            sdlaud_ringbuf_clear(ringbuf);
        }
    } else {
        applog_e("SDLAud", "Opus error from decode: %d", decodeLen);
    }
}

static void sdlaud_callback(void *userdata, Uint8 *stream, int len) {
    (void) userdata;
    if (readbuf_size < len) {
        readbuf = SDL_realloc(readbuf, len);
    }
    size_t read_size = sdlaud_ringbuf_read(ringbuf, readbuf, len);
    if (read_size > 0) {
        SDL_memset(stream, 0, len);
        SDL_MixAudio(stream, readbuf, read_size, SDL_MIX_MAXVOLUME);
    }
}

MODULE_API AUDIO_RENDERER_CALLBACKS audio_callbacks_sdl = {
        .init = setup,
        .cleanup = cleanup,
        .decodeAndPlaySample = queue,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};

MODULE_API bool audio_init_sdl(int argc, char *argv[], const HOST_CONTEXT *host) {
    if (host) {
        MODULE_LOGVPRINTF = host->logvprintf;
    }
    return true;
}

MODULE_API bool audio_check_sdl(PAUDIO_INFO ainfo) {
    ainfo->valid = true;
    ainfo->configuration = AUDIO_CONFIGURATION_STEREO;
    return true;
}
