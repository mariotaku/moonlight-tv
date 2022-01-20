#pragma once

#include <stdbool.h>

#include "backend/pcmanager.h"

enum STREAMING_STATUS {
    STREAMING_NONE,
    STREAMING_CONNECTING,
    STREAMING_STREAMING,
    STREAMING_DISCONNECTING,
    STREAMING_ERROR
};
typedef enum STREAMING_STATUS STREAMING_STATUS;

typedef enum streaming_interrupt_reason_t {
    STREAMING_INTERRUPT_USER,
    STREAMING_INTERRUPT_WATCHDOG,
    STREAMING_INTERRUPT_NETWORK,
    STREAMING_INTERRUPT_DECODER,
    STREAMING_INTERRUPT_BACKGROUND,
    STREAMING_INTERRUPT_QUIT,
} streaming_interrupt_reason_t;

typedef struct VIDEO_STATS {
    uint32_t totalFrames;
    uint32_t receivedFrames;
    uint32_t networkDroppedFrames;
    uint32_t decodedFrames;
    uint32_t totalReassemblyTime;
    uint32_t totalDecodeTime;
    unsigned long measurementStartTimestamp;
    float totalFps;
    float receivedFps;
    float decodedFps;
    uint32_t rtt, rttVariance;
} VIDEO_STATS;

typedef struct VIDEO_INFO {
    const char *format;
    int width;
    int height;
} VIDEO_INFO;

typedef struct AUDIO_STATS {
    uint32_t avgBufferTime;
} AUDIO_STATS;

extern STREAMING_STATUS streaming_status;
extern int streaming_errno;
extern char streaming_errmsg[];
extern short streaming_display_width, streaming_display_height;

void streaming_init();

void streaming_destroy();

bool streaming_running();

int streaming_begin(const SERVER_DATA *server, const APP_LIST *app);

void streaming_interrupt(bool quitapp, streaming_interrupt_reason_t reason);

void streaming_wait_for_stop();

void streaming_display_size(short width, short height);

void streaming_enter_fullscreen();

void streaming_enter_overlay(int x, int y, int w, int h);

void streaming_error(int code, const char *fmt, ...);

/**
 * Start a timer to interrupt stream if no video frame received after a period of time
 */
void streaming_watchdog_start();

void streaming_watchdog_stop();

void streaming_watchdog_reset();