#pragma once

#include "../types.h"
#include "client.h"
#include "executor.h"
#include "uuidstr.h"

typedef struct app_t app_t;
typedef struct apploader_task_t apploader_task_t;

typedef enum apploader_state_t {
    APPLOADER_STATE_IDLE,
    APPLOADER_STATE_LOADING,
    APPLOADER_STATE_ERROR,
} apploader_state_t;

typedef struct apploader_item_t {
    APP_LIST base;
    bool fav;
} apploader_item_t;

typedef struct apploader_list_t {
    size_t count;
    apploader_item_t *items;
} apploader_list_t;

typedef struct apploader_t apploader_t;

typedef struct apploader_cb_t {
    void (*start)(void *userdata);

    void (*data)(apploader_list_t *apps, void *userdata);

    void (*error)(int code, const char *error, void *userdata);
} apploader_cb_t;

typedef void (*apploader_cb)(apploader_t *loader, void *userdata);

apploader_t *apploader_create(app_t *global, const uuidstr_t *uuid, const apploader_cb_t *cb, void *userdata);

void apploader_destroy(apploader_t *loader);

void apploader_load(apploader_t *loader);

void apploader_cancel(apploader_t *loader);

apploader_state_t apploader_state(apploader_t *loader);

void apploader_list_free(apploader_list_t *list);

const apploader_item_t* apploader_list_item_by_id(const apploader_list_t *list, int id);