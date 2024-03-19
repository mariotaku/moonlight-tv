#pragma once

typedef struct session_t session_t;

int session_worker(session_t *session);

int session_worker_embedded(session_t *session);