#include "delegate.h"

#include <stddef.h>
#include <memory.h>
#include <time.h>

#include "stream/session.h"
#include "stream/module/api.h"

#include "util/bus.h"
#include "ui/streaming/streaming.controller.h"

static PDECODER_RENDERER_CALLBACKS vdec;
static int lastFrameNumber;
static struct VIDEO_STATS vdec_temp_stats;
struct VIDEO_STATS vdec_summary_stats;
struct VIDEO_INFO vdec_stream_info;

static int vdec_delegate_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags);

static void vdec_delegate_cleanup();

static int vdec_delegate_submit(PDECODE_UNIT decodeUnit);

static void vdec_stat_submit(const struct VIDEO_STATS *src, long now);

DECODER_RENDERER_CALLBACKS decoder_render_callbacks_delegate(PDECODER_RENDERER_CALLBACKS cb) {
    DECODER_RENDERER_CALLBACKS vdec_delegate = {
            .setup = vdec_delegate_setup,
            .start = cb->start,
            .stop = cb->stop,
            .cleanup = vdec_delegate_cleanup,
            .submitDecodeUnit = vdec_delegate_submit,
            .capabilities = cb->capabilities,
    };
    return vdec_delegate;
}

static const char *video_format_name(int videoFormat) {
    switch (videoFormat) {
        case VIDEO_FORMAT_H264:
            return "H264";
        case VIDEO_FORMAT_H265:
            return "H265";
        case VIDEO_FORMAT_H265_MAIN10:
            return "H265 10bit";
        default:
            return "Unknown";
    }
}

int vdec_delegate_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags) {
    vdec = context;
    memset(&vdec_temp_stats, 0, sizeof(vdec_temp_stats));

    vdec_stream_info.format = video_format_name(videoFormat);
    vdec_stream_info.width = width;
    vdec_stream_info.height = height;
    lastFrameNumber = 0;
    return vdec->setup(videoFormat, width, height, redrawRate, context, drFlags);
}

void vdec_delegate_cleanup() {
    vdec->cleanup();
    vdec = NULL;
}

int vdec_delegate_submit(PDECODE_UNIT decodeUnit) {
    static struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long ticksms = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    if (lastFrameNumber <= 0) {
        vdec_temp_stats.measurementStartTimestamp = ticksms;
        lastFrameNumber = decodeUnit->frameNumber;
    } else {
        // Any frame number greater than m_LastFrameNumber + 1 represents a dropped frame
        vdec_temp_stats.networkDroppedFrames += decodeUnit->frameNumber - (lastFrameNumber + 1);
        vdec_temp_stats.totalFrames += decodeUnit->frameNumber - (lastFrameNumber + 1);
        lastFrameNumber = decodeUnit->frameNumber;
    }
    // Flip stats windows roughly every second
    if (ticksms - vdec_temp_stats.measurementStartTimestamp > 1000) {
        vdec_stat_submit(&vdec_temp_stats, ticksms);

        // Move this window into the last window slot and clear it for next window
        memset(&vdec_temp_stats, 0, sizeof(vdec_temp_stats));
        vdec_temp_stats.measurementStartTimestamp = ticksms;
    }

    vdec_temp_stats.receivedFrames++;
    vdec_temp_stats.totalFrames++;

    vdec_temp_stats.totalReassemblyTime += decodeUnit->enqueueTimeMs - decodeUnit->receiveTimeMs;
    int err = vdec->submitDecodeUnit(decodeUnit);
    if (err == DR_OK) {
        vdec_temp_stats.totalDecodeTime += LiGetMillis() - decodeUnit->enqueueTimeMs;
        vdec_temp_stats.decodedFrames++;
    } else if (err == DR_INTERRUPT) {
        streaming_interrupt(false);
        err = DR_OK;
    }
    return err;
}

void vdec_stat_submit(const struct VIDEO_STATS *src, long now) {
    struct VIDEO_STATS *dst = &vdec_summary_stats;
    memcpy(dst, src, sizeof(struct VIDEO_STATS));
    dst->totalFps = (float) dst->totalFrames / ((float) (now - dst->measurementStartTimestamp) / 1000);
    dst->receivedFps = (float) dst->receivedFrames / ((float) (now - dst->measurementStartTimestamp) / 1000);
    dst->decodedFps = (float) dst->decodedFrames / ((float) (now - dst->measurementStartTimestamp) / 1000);
    LiGetEstimatedRttInfo(&dst->rtt, &dst->rttVariance);
    bus_pushaction((bus_actionfunc) streaming_refresh_stats, NULL);
}