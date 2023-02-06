#include "cec_support.h"
#include "app.h"

#include <assert.h>
#include <stdio.h>

#include <SDL.h>

#include "ceccloader.h"

#include "cec_key.h"
#include "util/logging.h"

static int cec_thread_worker(void *arg);

static void reopen_first(cec_support_ctx_t *ctx);

static void cb_alert(void *cbparam, libcec_alert alert, libcec_parameter param);

static ICECCallbacks cec_callbacks = {
        .alert = cb_alert,
        .keyPress = cec_support_cb_key,
};

cec_support_ctx_t *cec_support_create() {
    cec_support_ctx_t *ctx = calloc(1, sizeof(cec_support_ctx_t));
    ctx->lock = SDL_CreateMutex();
    SDL_LockMutex(ctx->lock);
    ctx->cond = SDL_CreateCond();
    ctx->cond_lock = SDL_CreateMutex();
    ctx->cec_iface = calloc(1, sizeof(libcec_interface_t));
    ctx->thread = SDL_CreateThread(cec_thread_worker, "cec-worker", ctx);
    SDL_UnlockMutex(ctx->lock);
    return ctx;
}

void cec_support_destroy(cec_support_ctx_t *ctx) {
    SDL_LockMutex(ctx->lock);
    ctx->quit = true;
    SDL_CondSignal(ctx->cond);
    SDL_WaitThread(ctx->thread, NULL);
    libcecc_destroy(ctx->cec_iface);
    SDL_UnlockMutex(ctx->lock);

    SDL_DestroyMutex(ctx->cond_lock);
    SDL_DestroyCond(ctx->cond);
    SDL_DestroyMutex(ctx->lock);
    free(ctx->cec_iface);
    free(ctx);
}

static int cec_thread_worker(void *arg) {
    cec_support_ctx_t *ctx = arg;
    libcec_configuration cec_conf;
    libcecc_reset_configuration(&cec_conf);
    cec_conf.clientVersion = LIBCEC_VERSION_CURRENT;
    cec_conf.bActivateSource = 0;
    cec_conf.callbacks = &cec_callbacks;
    cec_conf.callbackParam = ctx;
    snprintf(cec_conf.strDeviceName, sizeof(cec_conf.strDeviceName), "IHSplay");
    cec_conf.deviceTypes.types[0] = CEC_DEVICE_TYPE_PLAYBACK_DEVICE;

    if (libcecc_initialise(&cec_conf, ctx->cec_iface, NULL) != 1) {
        applog_i("CEC", "failed to initialize libcecc: %s", dlerror());
        goto cleanup;
    }
    applog_i("CEC", "libcecc initialized");
    reopen_first(ctx);
    SDL_LockMutex(ctx->cond_lock);
    while (!ctx->quit) {
        SDL_CondWait(ctx->cond, ctx->cond_lock);
    }
    SDL_UnlockMutex(ctx->cond_lock);

    cleanup:
    return 0;
}

static void cb_alert(void *cbparam, const libcec_alert alert, const libcec_parameter param) {

}

static void reopen_first(cec_support_ctx_t *ctx) {
    cec_adapter devices[8];
    libcec_interface_t *iface = ctx->cec_iface;
    uint8_t devices_found = iface->find_adapters(iface->connection, devices, 8, NULL);
    if (devices_found == 0) return;
    applog_i("CEC", "Opened CEC device %s", devices[0].path);
    iface->open(iface->connection, devices[0].comm, 5000);
}