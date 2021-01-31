#include "delegate.h"
#include "stream/session.h"

#include <stddef.h>
#include <memory.h>
#include <time.h>

static PDECODER_RENDERER_CALLBACKS vdec;
static int lastFrameNumber;
static struct VIDEO_STATS vdec_temp_stats;
struct VIDEO_STATS vdec_summary_stats;

static int _vdec_delegate_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags);
static void _vdec_delegate_cleanup();
static int _vdec_delegate_submit(PDECODE_UNIT decodeUnit);
static void _vdec_stat_submit(struct VIDEO_STATS *dst, long now);

DECODER_RENDERER_CALLBACKS decoder_render_callbacks_delegate(PDECODER_RENDERER_CALLBACKS cb)
{
    DECODER_RENDERER_CALLBACKS vdec_delegate = {
        .setup = _vdec_delegate_setup,
        .start = cb->start,
        .stop = cb->stop,
        .cleanup = _vdec_delegate_cleanup,
        .submitDecodeUnit = _vdec_delegate_submit,
        .capabilities = cb->capabilities};
    return vdec_delegate;
}

int _vdec_delegate_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    vdec = context;
    memset(&vdec_temp_stats, 0, sizeof(vdec_temp_stats));
    lastFrameNumber = 0;
    return vdec->setup(videoFormat, width, height, redrawRate, context, drFlags);
}

void _vdec_delegate_cleanup()
{
    vdec->cleanup();
    vdec = NULL;
}

int _vdec_delegate_submit(PDECODE_UNIT du)
{
    static struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long ticksms = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    if (lastFrameNumber <= 0)
    {
        vdec_temp_stats.measurementStartTimestamp = ticksms;
        lastFrameNumber = du->frameNumber;
    }
    else
    {
        // Any frame number greater than m_LastFrameNumber + 1 represents a dropped frame
        vdec_temp_stats.networkDroppedFrames += du->frameNumber - (lastFrameNumber + 1);
        vdec_temp_stats.totalFrames += du->frameNumber - (lastFrameNumber + 1);
        lastFrameNumber = du->frameNumber;
    }
    // Flip stats windows roughly every second
    if (ticksms - vdec_temp_stats.measurementStartTimestamp > 1000)
    {
        _vdec_stat_submit(&vdec_temp_stats, ticksms);

        // Move this window into the last window slot and clear it for next window
        memset(&vdec_temp_stats, 0, sizeof(vdec_temp_stats));
        vdec_temp_stats.measurementStartTimestamp = ticksms;
    }

    vdec_temp_stats.receivedFrames++;
    vdec_temp_stats.totalFrames++;

    vdec_temp_stats.totalReassemblyTime += du->enqueueTimeMs - du->receiveTimeMs;
    int err = vdec->submitDecodeUnit(du);
    if (err == 0)
    {
        vdec_temp_stats.totalDecodeTime += LiGetMillis() - du->enqueueTimeMs;
        vdec_temp_stats.decodedFrames++;
    }
    return err;
}

void _vdec_stat_submit(struct VIDEO_STATS *src, long now)
{
    struct VIDEO_STATS *dst = &vdec_summary_stats;
    memcpy(dst, src, sizeof(struct VIDEO_STATS));
    dst->totalFps = (float)dst->totalFrames / ((float)(now - dst->measurementStartTimestamp) / 1000);
    dst->receivedFps = (float)dst->receivedFrames / ((float)(now - dst->measurementStartTimestamp) / 1000);
    dst->decodedFps = (float)dst->decodedFrames / ((float)(now - dst->measurementStartTimestamp) / 1000);
}