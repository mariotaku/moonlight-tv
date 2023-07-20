#pragma once

#include "Limelight.h"

#define CALLBACKS_SESSION_ERROR_VDEC_UNSUPPORTED 9001
#define CALLBACKS_SESSION_ERROR_VDEC_ERROR 9002
#define CALLBACKS_SESSION_ERROR_ADEC_UNSUPPORTED 10001
#define CALLBACKS_SESSION_ERROR_ADEC_ERROR 10002

typedef struct session_t session_t;

CONNECTION_LISTENER_CALLBACKS *session_connection_callbacks_prepare(session_t *session);

void session_connection_callbacks_reset(session_t *session);