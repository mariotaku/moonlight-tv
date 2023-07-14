#pragma once

#include "uuidstr.h"

typedef struct app_t app_t;

int app_session_begin(app_t *app, const uuidstr_t *uuid, int app_id);

void app_session_destroy(app_t *app);