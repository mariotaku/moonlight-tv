#include "app.h"
#include "config.h"

#include "stream/session.h"
#include "stream/settings.h"

#include "stream/platform.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "stream/input/absinput.h"
#include "stream/video/delegate.h"
#include "backend/pcmanager/priv.h"

#include <Limelight.h>

#include "libgamestream/errors.h"

#include "util/logging.h"

extern CONNECTION_LISTENER_CALLBACKS connection_callbacks;

STREAMING_STATUS streaming_status = STREAMING_NONE;
int streaming_errno = GS_OK;
char streaming_errmsg[1024];

short streaming_display_width, streaming_display_height;

static SDL_mutex *streaming_state_lock;
static SDL_Thread *streaming_thread;

typedef struct {
    /* SERVER_DATA and CONFIGURATION is cloned rather than referenced */
    SERVER_DATA *server;
    CONFIGURATION *config;
    int appId;
    bool interrupted;
    bool quitapp;
    SDL_cond *cond;
    SDL_mutex *mutex;
    SDL_Thread *thread;
    PVIDEO_PRESENTER_CALLBACKS pres;
} session_t;

static session_t *session_active;

static int streaming_worker(session_t *session);

static void streaming_set_status(STREAMING_STATUS status);

static bool streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps);

void streaming_init() {
    streaming_status = STREAMING_NONE;
    streaming_state_lock = SDL_CreateMutex();
}

void streaming_destroy() {
    streaming_interrupt(false, STREAMING_INTERRUPT_QUIT);
    streaming_wait_for_stop();

    SDL_DestroyMutex(streaming_state_lock);
}

bool streaming_running() {
    return streaming_status == STREAMING_STREAMING;
}

int streaming_begin(const SERVER_DATA *server, const APP_LIST *app) {
    if (session_active != NULL) {
        return -1;
    }
    PSERVER_DATA server_clone = serverdata_clone(server);
    PCONFIGURATION config = settings_load();

    if (config->stream.bitrate < 0) {
        config->stream.bitrate = settings_optimal_bitrate(config->stream.width, config->stream.height,
                                                          config->stream.fps);
    }
    // Cap framerate to platform request
    if (decoder_info.maxBitrate && config->stream.bitrate > decoder_info.maxBitrate)
        config->stream.bitrate = decoder_info.maxBitrate;
    config->sops &= streaming_sops_supported(server_clone->modes, config->stream.width, config->stream.height,
                                             config->stream.fps);
    config->stream.supportsHevc &= decoder_info.hevc;
    config->stream.enableHdr &= decoder_info.hevc && decoder_info.hdr && server_clone->supportsHdr &&
                                (decoder_info.hdr == DECODER_HDR_ALWAYS || app->hdr != 0);
    config->stream.colorSpace = decoder_info.colorSpace;
    if (config->stream.enableHdr) {
        config->stream.colorRange = decoder_info.colorRange;
    }
    applog_i("Session", "enableHdr=%u", config->stream.enableHdr);
    applog_i("Session", "colorSpace=%d", config->stream.colorSpace);
    applog_i("Session", "colorRange=%d", config->stream.colorRange);
#if FEATURE_SURROUND_SOUND
    if (CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(module_audio_configuration()) <
        CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config->stream.audioConfiguration)) {
        config->stream.audioConfiguration = module_audio_configuration();
    }
    if (!config->stream.audioConfiguration) {
        config->stream.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
    }
#endif
    config->stream.encryptionFlags = ENCFLG_AUDIO;

    session_t *req = malloc(sizeof(session_t));
    SDL_memset(req, 0, sizeof(session_t));
    req->server = server_clone;
    req->config = config;
    req->appId = app->id;
    req->mutex = SDL_CreateMutex();
    req->cond = SDL_CreateCond();
    req->thread = SDL_CreateThread((SDL_ThreadFunction) streaming_worker, "session", req);
    session_active = req;
    return 0;
}

void streaming_interrupt(bool quitapp, streaming_interrupt_reason_t reason) {
    session_t *session = session_active;
    if (!session || session->interrupted) {
        return;
    }
    SDL_LockMutex(session->mutex);
    session->quitapp = quitapp;
    session->interrupted = true;
    if (reason >= STREAMING_INTERRUPT_ERROR) {
        switch (reason) {
            case STREAMING_INTERRUPT_NETWORK:
                streaming_error(reason, "Network error happened");
                break;
            case STREAMING_INTERRUPT_WATCHDOG:
                streaming_error(reason, "Stream stalled");
                break;
            case STREAMING_INTERRUPT_DECODER:
                streaming_error(reason, "Decoder reported error");
                break;
            default:
                streaming_error(reason, "Error occurred while in streaming");
                break;
        }
    }
    SDL_CondSignal(session->cond);
    SDL_UnlockMutex(session->mutex);
}

void streaming_wait_for_stop() {
    if (streaming_thread && streaming_status != STREAMING_NONE) {
        SDL_WaitThread(streaming_thread, NULL);
    }
}

void streaming_display_size(short width, short height) {
    streaming_display_width = width;
    streaming_display_height = height;
}

int streaming_worker(session_t *session) {
    streaming_set_status(STREAMING_CONNECTING);
    bus_pushevent(USER_STREAM_CONNECTING, NULL, NULL);
    streaming_error(GS_OK, "");
    PSERVER_DATA server = session->server;
    PCONFIGURATION config = session->config;
    absinput_no_control = config->viewonly;
    absinput_set_virtual_mouse(false);
    int appId = session->appId;

    int gamepads = absinput_gamepads();
    int gamepad_mask = 0;
    for (int i = 0; i < gamepads && i < 4; i++)
        gamepad_mask = (gamepad_mask << 1) + 1;

    applog_i("Session", "Launch app %d...", appId);
    GS_CLIENT client = app_gs_client_new();
    gs_set_timeout(client, 30);
    int ret = gs_start_app(client, server, &config->stream, appId, config->sops, config->localaudio, gamepad_mask);
    if (ret != GS_OK) {
        streaming_set_status(STREAMING_ERROR);
        if (gs_error) {
            streaming_error(ret, "Failed to launch session: %s (code %d)", gs_error, ret);
        } else {
            streaming_error(ret, "Failed to launch session: gamestream returned %d", ret);
        }
        applog_e("Session", "Failed to launch session: gamestream returned %d, gs_error=%s", ret, gs_error);
        goto thread_cleanup;
    }

    applog_i("Session", "Video %d x %d, %d net_fps, %d kbps", config->stream.width, config->stream.height,
             config->stream.fps, config->stream.bitrate);
    applog_i("Session", "Audio %d channels", CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config->stream.audioConfiguration));

    PDECODER_RENDERER_CALLBACKS vdec = decoder_get_video();
    PAUDIO_RENDERER_CALLBACKS adec = module_get_audio(config->audio_device);
    PVIDEO_PRESENTER_CALLBACKS pres = decoder_get_presenter();
    DECODER_RENDERER_CALLBACKS vdec_delegate = decoder_render_callbacks_delegate(vdec);

    int startResult = LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks,
                                        &vdec_delegate, adec, vdec, 0, (void *) config->audio_device, 0);
    if (startResult != 0) {
        streaming_set_status(STREAMING_ERROR);
        switch (startResult) {
            case ERROR_UNKNOWN_CODEC:
                streaming_error(GS_WRONG_STATE, "Unsupported codec.");
                break;
            case ERROR_DECODER_OPEN_FAILED:
                streaming_error(GS_WRONG_STATE, "Failed to open video decoder.");
                break;
            case ERROR_AUDIO_OPEN_FAILED:
                streaming_error(GS_WRONG_STATE, "Failed to open audio backend.");
                break;
            case ERROR_AUDIO_OPUS_INIT_FAILED:
                streaming_error(GS_WRONG_STATE, "Opus init failed.");
                break;
            default: {
                if (!streaming_errno) {
                    streaming_error(GS_WRONG_STATE, "Failed to start connection: Limelight returned %d (%s)",
                                    startResult, strerror(startResult));
                }
                break;
            }
        }
        applog_e("Session", "Failed to start connection: Limelight returned %d", startResult);
        goto thread_cleanup;
    }
    streaming_set_status(STREAMING_STREAMING);
    if (config->stop_on_stall) {
        streaming_watchdog_start();
    }
    session->pres = pres;
    bus_pushevent(USER_STREAM_OPEN, &config->stream, NULL);
    SDL_LockMutex(session->mutex);
    while (!session->interrupted) {
        // Wait until interrupted
        SDL_CondWait(session->cond, session->mutex);
    }
    SDL_UnlockMutex(session->mutex);
    streaming_watchdog_stop();
    session->pres = NULL;
    bus_pushevent(USER_STREAM_CLOSE, NULL, NULL);

    streaming_set_status(STREAMING_DISCONNECTING);
    LiStopConnection();

    if (session->quitapp) {
        applog_i("Session", "Sending app quit request ...");
        gs_quit_app(client, server);
    }
    pcmanager_upsert_worker(pcmanager, server->serverInfo.address, true, NULL, NULL);

    // Don't always reset status as error state should be kept
    streaming_set_status(STREAMING_NONE);
    thread_cleanup:
    gs_destroy(client);
    bus_pushevent(USER_STREAM_FINISHED, NULL, NULL);
    session->pres = NULL;
    serverdata_free(session->server);
    settings_free(config);
    SDL_DestroyCond(session->cond);
    SDL_DestroyMutex(session->mutex);
    free(session);
    session_active = NULL;
    return 0;
}

void streaming_enter_fullscreen() {
    app_set_mouse_grab(true);
    if (session_active->pres && session_active->pres->enterFullScreen) {
        session_active->pres->enterFullScreen();
    }
}

void streaming_enter_overlay(int x, int y, int w, int h) {
    app_set_mouse_grab(false);
    if (session_active->pres && session_active->pres->enterOverlay) {
        session_active->pres->enterOverlay(x, y, w, h);
    }
}

void streaming_set_hdr(bool hdr) {
    if (session_active->pres && session_active->pres->setHdr) {
        session_active->pres->setHdr(hdr);
    }
}

void streaming_set_status(STREAMING_STATUS status) {
    SDL_LockMutex(streaming_state_lock);
    streaming_status = status;
    SDL_UnlockMutex(streaming_state_lock);
}

void streaming_error(int code, const char *fmt, ...) {
    SDL_LockMutex(streaming_state_lock);
    streaming_errno = code;
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(streaming_errmsg, sizeof(streaming_errmsg) / sizeof(char), fmt, arglist);
    va_end(arglist);
    SDL_UnlockMutex(streaming_state_lock);
}

bool streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps) {
    for (PDISPLAY_MODE cur = modes; cur != NULL; cur = cur->next) {
        if (cur->width == w && cur->height == h && cur->refresh == fps)
            return true;
    }
    return false;
}