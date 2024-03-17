#pragma once

#include "uuidstr.h"
#include "client.h"

typedef struct app_t app_t;

int app_session_begin(app_t *app, const uuidstr_t *uuid, const APP_LIST *gs_app);

void app_session_destroy(app_t *app);