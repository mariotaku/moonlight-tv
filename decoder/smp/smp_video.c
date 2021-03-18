#include "video.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "smp_common.h"

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE 2048 * 1024

static unsigned char *smp_buffer = NULL;

static int _setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    smp_buffer = malloc(DECODER_BUFFER_SIZE);
    if (smp_buffer == NULL)
    {
        fprintf(stderr, "Not enough memory\n");
        return -1;
    }
    memcpy(&videoConfig, &(DirectMediaVideoConfig){DirectMediaVideoCodecH264, width, height}, sizeof(DirectMediaVideoConfig));
    smp_player_create();
    return 0;
}

static void _start()
{
    smp_player_open();
}

static void _stop()
{
    smp_player_close();
}

static void _cleanup()
{
    smp_player_destroy();
    if (smp_buffer)
    {
        free(smp_buffer);
        smp_buffer = NULL;
    }
}

static int _submit_decode_unit(PDECODE_UNIT decodeUnit)
{
    if (!playerctx || !smp_buffer)
        return DR_OK;
    unsigned long long ms = decodeUnit->presentationTimeMs;
    pts = ms * 1000000ULL;
    if (decodeUnit->fullLength < DECODER_BUFFER_SIZE)
    {
        PLENTRY entry = decodeUnit->bufferList;
        int length = 0;
        for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next)
        {
            memcpy(smp_buffer + length, entry->data, entry->length);
            length += entry->length;
        }
        StarfishDirectMediaPlayer_Feed(playerctx, smp_buffer, length, pts, DirectMediaFeedVideo);
    }
    else
    {
        fprintf(stderr, "Video decode buffer too small, skip this frame");
        return DR_NEED_IDR;
    }
    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_smp = {
    .setup = _setup,
    .start = _start,
    .stop = _stop,
    .cleanup = _cleanup,
    .submitDecodeUnit = _submit_decode_unit,
    .capabilities = CAPABILITY_SLICES_PER_FRAME(4) | CAPABILITY_DIRECT_SUBMIT,
};
