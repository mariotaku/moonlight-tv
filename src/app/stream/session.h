#pragma once

#include <stdbool.h>

#include "backend/pcmanager.h"

enum STREAMING_STATE {
    STREAMING_NONE,
    STREAMING_CONNECTING,
    STREAMING_STREAMING,
    STREAMING_DISCONNECTING,
    STREAMING_ERROR
};
typedef enum STREAMING_STATE STREAMING_STATE;

typedef enum streaming_interrupt_reason_t {
    STREAMING_INTERRUPT_USER,
    STREAMING_INTERRUPT_BACKGROUND,
    STREAMING_INTERRUPT_QUIT,
    STREAMING_INTERRUPT_HOST,
    STREAMING_INTERRUPT_ERROR = 0x1000,
    STREAMING_INTERRUPT_WATCHDOG,
    STREAMING_INTERRUPT_NETWORK,
    STREAMING_INTERRUPT_DECODER,
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

typedef struct session_config_t {
    STREAM_CONFIGURATION stream;
    bool sops;
    bool view_only;
    bool local_audio;
    bool hardware_mouse;
    bool vmouse;
} session_config_t;

extern int streaming_errno;
extern char streaming_errmsg[];

typedef struct app_t app_t;
typedef struct app_configuration_t app_configuration_t;
typedef struct session_t session_t;

session_t *session_create(app_t *app, const app_configuration_t *config, const SERVER_DATA *server, int app_id);

void session_destroy(session_t *session);

void session_interrupt(session_t *session, bool quitapp, streaming_interrupt_reason_t reason);

void session_start_input(session_t *session);

void session_stop_input(session_t *session);

void session_toggle_vmouse(session_t *session);

bool session_accepting_input(session_t *session);

void streaming_display_size(session_t *session, short width, short height);

void streaming_enter_fullscreen(session_t *session);

void streaming_enter_overlay(session_t *session, int x, int y, int w, int h);

void streaming_set_hdr(session_t *session, bool hdr);

void streaming_error(session_t *session, int code, const char *fmt, ...);
