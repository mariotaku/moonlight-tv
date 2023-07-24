#pragma once

#include <stdbool.h>

struct img_loader_t;
struct img_loader_task_t;

typedef struct img_loader_t img_loader_t;
typedef struct img_loader_task_t img_loader_task_t;
typedef struct img_loader_req_t img_loader_req_t;

typedef void (*img_loader_fn)(img_loader_req_t *req);

typedef bool (*img_loader_get_fn)(img_loader_req_t *req);

typedef void (*img_loader_run_on_main_fn)(void *args);

typedef struct lv_img_loader_cb_t {
    img_loader_fn start_cb;

    img_loader_fn fail_cb;

    img_loader_fn complete_cb;

    img_loader_fn cancel_cb;
} img_loader_cb_t;

typedef struct img_loader_datasource_t {
    img_loader_get_fn memcache_get;

    img_loader_fn memcache_put;

    img_loader_get_fn filecache_get;

    img_loader_fn filecache_put;

    img_loader_get_fn fetch;

    void (*run_on_main)(img_loader_t *loader, img_loader_run_on_main_fn fn, void *args);
} img_loader_impl_t;

img_loader_t *img_loader_create(const img_loader_impl_t *impl, executor_t*executor);

void img_loader_destroy(img_loader_t *loader);

img_loader_task_t *img_loader_load(img_loader_t *loader, img_loader_req_t *request, const img_loader_cb_t *cb);

void img_loader_cancel(img_loader_t *loader, img_loader_task_t *task);