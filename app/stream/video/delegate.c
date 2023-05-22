#include "delegate.h"

#include <stddef.h>

#include "stream/session.h"

#include "sps_parser.h"

#include "ui/streaming/streaming.controller.h"
#include "util/bus.h"
#include "logging.h"
#include "ss4s.h"

#include <SDL.h>
#include <assert.h>

// 2MB decode size should be fairly enough for everything
#define DECODER_BUFFER_SIZE (2048 * 1024)

static SS4S_Player *player = NULL;
static unsigned char *buffer = NULL;
static int lastFrameNumber;
static struct VIDEO_STATS vdec_temp_stats;
static int vdec_stream_format = 0;
VIDEO_STATS vdec_summary_stats;
VIDEO_INFO vdec_stream_info;

static int vdec_delegate_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags);

static void vdec_delegate_cleanup();

static int vdec_delegate_submit(PDECODE_UNIT decodeUnit);

static void vdec_stat_submit(const struct VIDEO_STATS *src, unsigned long now);

static void stream_info_parse_size(PDECODE_UNIT decodeUnit, struct VIDEO_INFO *info);

DECODER_RENDERER_CALLBACKS ss4s_dec_callbacks = {
        .setup = vdec_delegate_setup,
        .cleanup = vdec_delegate_cleanup,
        .submitDecodeUnit = vdec_delegate_submit,
        .capabilities = CAPABILITY_DIRECT_SUBMIT,
};

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
    (void) redrawRate;
    (void) drFlags;
    player = context;
    buffer = malloc(DECODER_BUFFER_SIZE);
    memset(&vdec_temp_stats, 0, sizeof(vdec_temp_stats));
    vdec_stream_format = videoFormat;
    vdec_stream_info.format = video_format_name(videoFormat);
    lastFrameNumber = 0;
    SS4S_VideoInfo info = {
            .width = width,
            .height = height,
    };
    switch (videoFormat) {
        case VIDEO_FORMAT_H264:
            info.codec = SS4S_VIDEO_H264;
            break;
        case VIDEO_FORMAT_H265:
        case VIDEO_FORMAT_H265_MAIN10:
            info.codec = SS4S_VIDEO_H265;
            break;
        default: {
            return -1;
        }
    }
    return SS4S_PlayerVideoOpen(player, &info);
}

void vdec_delegate_cleanup() {
    assert(player != NULL);
    free(buffer);
    SS4S_PlayerVideoClose(player);
}

int vdec_delegate_submit(PDECODE_UNIT decodeUnit) {
    if (decodeUnit->fullLength > DECODER_BUFFER_SIZE) {
        return 0;
    }
    unsigned long ticksms = SDL_GetTicks();
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
    size_t length = 0;
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next) {
        memcpy(buffer + length, entry->data, entry->length);
        length += entry->length;
    }
    SS4S_VideoFeedResult result = SS4S_PlayerVideoFeed(player, buffer, length, 0);
    int err;
    if (result == SS4S_VIDEO_FEED_OK) {
        if (vdec_stream_info.width == 0 || vdec_stream_info.height == 0) {
            stream_info_parse_size(decodeUnit, &vdec_stream_info);
        }
        vdec_temp_stats.totalDecodeTime += LiGetMillis() - decodeUnit->enqueueTimeMs;
        vdec_temp_stats.decodedFrames++;
        streaming_watchdog_reset();
        return DR_OK;
    } else if (result == SS4S_VIDEO_FEED_ERROR) {
        streaming_interrupt(false, STREAMING_INTERRUPT_DECODER);
        return DR_OK;
    } else {
        return DR_NEED_IDR;
    }
}

void vdec_stat_submit(const struct VIDEO_STATS *src, unsigned long now) {
    struct VIDEO_STATS *dst = &vdec_summary_stats;
    memcpy(dst, src, sizeof(struct VIDEO_STATS));
    unsigned long delta = now - dst->measurementStartTimestamp;
    if (delta <= 0) return;
    dst->totalFps = (float) dst->totalFrames / ((float) delta / 1000);
    dst->receivedFps = (float) dst->receivedFrames / ((float) delta / 1000);
    dst->decodedFps = (float) dst->decodedFrames / ((float) delta / 1000);
    LiGetEstimatedRttInfo(&dst->rtt, &dst->rttVariance);
    bus_pushaction((bus_actionfunc) streaming_refresh_stats, NULL);
}

void stream_info_parse_size(PDECODE_UNIT decodeUnit, struct VIDEO_INFO *info) {
    for (PLENTRY entry = decodeUnit->bufferList; entry != NULL; entry = entry->next) {
        if (entry->bufferType != BUFFER_TYPE_SPS) continue;
        sps_dimension_t dimension;
        if (vdec_stream_format & VIDEO_FORMAT_MASK_H264) {
            sps_parse_dimension_h264((const unsigned char *) &entry->data[4], &dimension);
        } else if (vdec_stream_format & VIDEO_FORMAT_MASK_H265) {
            sps_parse_dimension_hevc((const unsigned char *) &entry->data[4], &dimension);
        } else {
            info->width = info->height = -1;
            return;
        }
        info->width = dimension.width;
        info->height = dimension.height;
        return;
    }
}