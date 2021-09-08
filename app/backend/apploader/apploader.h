#pragma once

#include "../types.h"
#include "client.h"

typedef enum apploader_status_t {
    APPLOADER_STATUS_IDLE,
    APPLOADER_STATUS_LOADING
} apploader_status_t;

typedef struct apploader_t {
    const SERVER_LIST *node;
    apploader_status_t status;
    int code;
    APP_LIST *apps;
    int apps_count;
} apploader_t;

typedef void (*apploader_cb)(apploader_t *loader, void *userdata);

apploader_t *apploader_new(const SERVER_LIST *node);

void apploader_load(apploader_t *loader, apploader_cb cb, void *userdata);

void apploader_destroy(apploader_t *loader);