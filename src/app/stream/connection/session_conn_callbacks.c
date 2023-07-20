#include "ui/streaming/streaming.controller.h"
#include "stream/input/session_input.h"

#include "util/i18n.h"
#include "logging.h"
#include "input/input_gamepad.h"
#include "app.h"
#include "stream/session_priv.h"

static session_t *current_session = NULL;

static void connection_terminated(int errorCode) {
    if (errorCode == ML_ERROR_GRACEFUL_TERMINATION) {
        session_interrupt(current_session, false, STREAMING_INTERRUPT_HOST);
    } else {
        commons_log_error("Session", "Connection terminated, errorCode = 0x%x", errorCode);
        streaming_error(current_session, 0, "Connection terminated, errorCode = 0x%x", errorCode);
        session_interrupt(current_session, false, STREAMING_INTERRUPT_NETWORK);
    }
}

static void connection_log_message(const char *format, ...) {
    va_list arglist;
    va_start(arglist, format);
    commons_log_vprintf(COMMONS_LOG_LEVEL_INFO, "Limelight", format, arglist);
    va_end(arglist);
}

static void connection_status_update(int status) {
    switch (status) {
        case CONN_STATUS_OKAY:
            commons_log_info("Session", "Connection is okay");
            streaming_notice_show(NULL);
            break;
        case CONN_STATUS_POOR:
            commons_log_warn("Session", "Connection is poor");
            streaming_notice_show(locstr("Unstable connection."));
            break;
        default:
            break;
    }
}

static void connection_stage_failed(int stage, int errorCode) {
    const char *stageName = LiGetStageName(stage);
    commons_log_error("Session", "Connection failed at stage %d (%s), errorCode = %d (%s)", stage, stageName, errorCode,
                      strerror(errorCode));
    streaming_error(current_session, errorCode, "Connection failed at stage %d (%s), errorCode = %d (%s)", stage,
                    stageName, errorCode, strerror(errorCode));
}

static void connection_rumble(unsigned short controllerNumber, unsigned short lowFreqMotor,
                              unsigned short highFreqMotor) {
    app_input_gamepad_rumble(&current_session->app->input, controllerNumber, lowFreqMotor, highFreqMotor);
}

static void connection_set_hdr(bool hdrEnabled) {
    streaming_set_hdr(current_session, hdrEnabled);
}

CONNECTION_LISTENER_CALLBACKS connection_callbacks = {
        .stageStarting = NULL,
        .stageComplete = NULL,
        .stageFailed = connection_stage_failed,
        .connectionStarted = NULL,
        .connectionTerminated = connection_terminated,
        .logMessage = connection_log_message,
        .rumble = connection_rumble,
        .connectionStatusUpdate = connection_status_update,
        .setHdrMode = connection_set_hdr
};


CONNECTION_LISTENER_CALLBACKS *session_connection_callbacks_prepare(session_t *session) {
    current_session = session;
    return &connection_callbacks;
}

void session_connection_callbacks_reset(session_t *session) {
    current_session = NULL;
}