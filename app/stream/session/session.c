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

#include "logging.h"
#include "stream/input/sdlinput.h"
#include "callbacks.h"
#include "ss4s.h"
#include "backend/pcmanager/worker/worker.h"

#if FEATURE_INPUT_EVMOUSE
#include "evmouse.h"
#include <errno.h>
#endif


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
    SS4S_VideoCapabilities video_cap;
    SDL_cond *cond;
    SDL_mutex *mutex;
    SDL_Thread *thread;
    SS4S_Player *player;
#if FEATURE_INPUT_EVMOUSE
    struct {
        SDL_Thread *thread;
        evmouse_t *dev;
    } mouse;
#endif
} session_t;

static session_t *session_active;

static int streaming_worker(session_t *session);

#if FEATURE_INPUT_EVMOUSE
static void mouse_listener(const evmouse_event_t *event, void *userdata);

static int mouse_worker(session_t *session);
#endif

static void streaming_set_status(STREAMING_STATUS status);

static bool streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps);

static void streaming_thread_wait(SDL_Thread *thread);

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

int streaming_begin(app_t *global, const uuidstr_t *uuid, const APP_LIST *app) {
    if (session_active != NULL) {
        return -1;
    }
    const pclist_t *node = pcmanager_node(pcmanager, uuid);
    if (node == NULL) {
        return -1;
    }
    PSERVER_DATA server_clone = serverdata_clone(node->server);
    PCONFIGURATION config = settings_load();

    SS4S_VideoCapabilities video_cap = global->ss4s.video_cap;
    SS4S_AudioCapabilities audio_cap = global->ss4s.audio_cap;

    if (config->stream.bitrate < 0) {
        config->stream.bitrate = settings_optimal_bitrate(&video_cap, config->stream.width, config->stream.height,
                                                          config->stream.fps);
    }
    // Cap framerate to platform request
    if (video_cap.maxBitrate && config->stream.bitrate > video_cap.maxBitrate)
        config->stream.bitrate = (int) video_cap.maxBitrate;
    config->sops &= streaming_sops_supported(server_clone->modes, config->stream.width, config->stream.height,
                                             config->stream.fps);
    config->stream.supportsHevc &= (video_cap.codecs & SS4S_VIDEO_H265) != 0;
    config->stream.enableHdr &= config->stream.supportsHevc && video_cap.hdr && server_clone->supportsHdr &&
                                (app->hdr != 0/* TODO: handle always on case*/);
    config->stream.colorSpace = COLORSPACE_REC_709/* TODO: get from video capabilities */;
    if (config->stream.enableHdr) {
        config->stream.colorRange = COLOR_RANGE_FULL/* TODO: get from video capabilities */;
    }
    commons_log_info("Session", "enableHdr=%u", config->stream.enableHdr);
    commons_log_info("Session", "colorSpace=%d", config->stream.colorSpace);
    commons_log_info("Session", "colorRange=%d", config->stream.colorRange);
#if FEATURE_SURROUND_SOUND
    if (audio_cap.maxChannels < CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config->stream.audioConfiguration)) {
        switch (audio_cap.maxChannels) {
            case 2:
                config->stream.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
                break;
            case 6:
                config->stream.audioConfiguration = AUDIO_CONFIGURATION_51_SURROUND;
                break;
            case 8:
                config->stream.audioConfiguration = AUDIO_CONFIGURATION_71_SURROUND;
                break;
        }
    }
    if (!config->stream.audioConfiguration) {
        config->stream.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
    }
#endif
    config->stream.encryptionFlags = ENCFLG_AUDIO;

    session_t *session = malloc(sizeof(session_t));
    SDL_memset(session, 0, sizeof(session_t));
    session->video_cap = global->ss4s.video_cap;
    session->server = server_clone;
    session->config = config;
    session->appId = app->id;
    session->mutex = SDL_CreateMutex();
    session->cond = SDL_CreateCond();
    session->thread = SDL_CreateThread((SDL_ThreadFunction) streaming_worker, "session", session);
    if (!config->viewonly && config->hardware_mouse) {
#if FEATURE_INPUT_EVMOUSE
        session->mouse.thread = SDL_CreateThread((SDL_ThreadFunction) mouse_worker, "sessinput", session);
#endif
    }
    session_active = session;
    return 0;
}

void streaming_interrupt(bool quitapp, streaming_interrupt_reason_t reason) {
    session_t *session = session_active;
    if (!session || session->interrupted) {
        return;
    }
    SDL_LockMutex(session->mutex);
#if FEATURE_INPUT_EVMOUSE
    if (session->mouse.dev != NULL) {
        evmouse_interrupt(session->mouse.dev);
    }
#endif
    session->quitapp = quitapp;
    session->interrupted = true;
    if (reason >= STREAMING_INTERRUPT_ERROR) {
        switch (reason) {
            case STREAMING_INTERRUPT_WATCHDOG:
                streaming_error(reason, "Stream stalled");
                break;
            case STREAMING_INTERRUPT_NETWORK:
                streaming_error(reason, "Network error happened");
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
    absinput_no_sdl_mouse = config->hardware_mouse;
    absinput_set_virtual_mouse(false);
    int appId = session->appId;

    int gamepad_mask = 0;
    for (int i = 0, cnt = absinput_gamepads(); i < cnt && i < 4; i++) {
        gamepad_mask = (gamepad_mask << 1) + 1;
    }
    session->player = NULL;

    commons_log_info("Session", "Launch app %d...", appId);
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
        commons_log_error("Session", "Failed to launch session: gamestream returned %d, gs_error=%s", ret, gs_error);
        goto thread_cleanup;
    }

    commons_log_info("Session", "Video %d x %d, %d net_fps, %d kbps", config->stream.width, config->stream.height,
                     config->stream.fps, config->stream.bitrate);
    commons_log_info("Session", "Audio %d channels",
                     CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config->stream.audioConfiguration));

    session->player = SS4S_PlayerOpen();
    SS4S_PlayerSetWaitAudioVideoReady(session->player, true);

    int startResult = LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks,
                                        &ss4s_dec_callbacks, &ss4s_aud_callbacks,
                                        session->player, 0, session->player, 0);
    if (startResult != 0) {
        streaming_set_status(STREAMING_ERROR);
        switch (startResult) {
            case CALLBACKS_SESSION_ERROR_VDEC_UNSUPPORTED:
                streaming_error(GS_WRONG_STATE, "Unsupported video codec.");
                break;
            case CALLBACKS_SESSION_ERROR_VDEC_ERROR:
                streaming_error(GS_WRONG_STATE, "Failed to open video decoder.");
                break;
            case CALLBACKS_SESSION_ERROR_ADEC_UNSUPPORTED:
                streaming_error(GS_WRONG_STATE, "Unsupported audio codec.");
                break;
            case CALLBACKS_SESSION_ERROR_ADEC_ERROR:
                streaming_error(GS_WRONG_STATE, "Failed to open audio backend.");
                break;
//            case ERROR_AUDIO_OPUS_INIT_FAILED:
//                streaming_error(GS_WRONG_STATE, "Opus init failed.");
//                break;
            default: {
                if (!streaming_errno) {
                    streaming_error(GS_WRONG_STATE, "Failed to start connection: Limelight returned %d (%s)",
                                    startResult, strerror(startResult));
                }
                break;
            }
        }
        commons_log_error("Session", "Failed to start connection: Limelight returned %d", startResult);
        goto thread_cleanup;
    }
    streaming_set_status(STREAMING_STREAMING);
    if (config->stop_on_stall) {
        streaming_watchdog_start();
    }
    bus_pushevent(USER_STREAM_OPEN, &config->stream, NULL);
    SDL_LockMutex(session->mutex);
    while (!session->interrupted) {
        // Wait until interrupted
        SDL_CondWait(session->cond, session->mutex);
    }
    SDL_UnlockMutex(session->mutex);
    streaming_watchdog_stop();
    bus_pushevent(USER_STREAM_CLOSE, NULL, NULL);

    streaming_set_status(STREAMING_DISCONNECTING);
    LiStopConnection();

    if (session->quitapp) {
        commons_log_info("Session", "Sending app quit request ...");
        gs_quit_app(client, server);
    }
    worker_context_t update_ctx = {
            .manager = pcmanager,
    };
    pcmanager_update_by_ip(&update_ctx, server->serverInfo.address, true);

    // Don't always reset status as error state should be kept
    streaming_set_status(STREAMING_NONE);
    thread_cleanup:
    if (session->player != NULL) {
        SS4S_PlayerClose(session->player);
    }
    gs_destroy(client);
    bus_pushevent(USER_STREAM_FINISHED, NULL, NULL);
    serverdata_free(session->server);
    settings_free(config);
    SDL_DestroyCond(session->cond);
    SDL_DestroyMutex(session->mutex);
#if FEATURE_INPUT_EVMOUSE
    if (session->mouse.thread != NULL) {
        SDL_WaitThread(session->mouse.thread, NULL);
    }
#endif
    SDL_Thread *thread = session->thread;
    free(session);
    session_active = NULL;
    bus_pushaction((bus_actionfunc) streaming_thread_wait, thread);
    return 0;
}

#if FEATURE_INPUT_EVMOUSE
static int mouse_worker(session_t *session) {
    session->mouse.dev = evmouse_open_default();
    if (session->mouse.dev == NULL) {
        commons_log_warn("Session", "No mouse device available");
        return ENODEV;
    }
    commons_log_info("Session", "EvMouse opened");
    evmouse_listen(session->mouse.dev, mouse_listener, session);
    evmouse_close(session->mouse.dev);
    commons_log_info("Session", "EvMouse closed");
    return 0;
}

static void mouse_listener(const evmouse_event_t *event, void *userdata) {
    if (!absinput_should_accept()) {
        return;
    }
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            sdlinput_handle_mbutton_event(&event->button);
            break;
        }
        case SDL_MOUSEMOTION:
            LiSendMouseMoveEvent((short) event->motion.xrel, (short) event->motion.yrel);
            break;
        case SDL_MOUSEWHEEL:
            sdlinput_handle_mwheel_event(&event->wheel);
            break;
    }
}
#endif

void streaming_enter_fullscreen() {
    if (session_active == NULL) {
        return;
    }
    app_set_mouse_grab(true);
    if ((session_active->video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_UI_COMPOSITING) == 0) {
        SS4S_PlayerVideoSetDisplayArea(session_active->player, NULL, NULL);
    }
}

void streaming_enter_overlay(int x, int y, int w, int h) {
    if (session_active == NULL) {
        return;
    }
    app_set_mouse_grab(false);
    SS4S_VideoRect dst = {x, y, w, h};
    if ((session_active->video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_UI_COMPOSITING) == 0) {
        SS4S_PlayerVideoSetDisplayArea(session_active->player, NULL, &dst);
    }
}

void streaming_set_hdr(bool hdr) {
    if (session_active == NULL) {
        return;
    }
    if (hdr) {
        SS4S_VideoHDRInfo info = {
                .displayPrimariesX = {13250, 7500, 34000},
                .displayPrimariesY = {34500, 3000, 16000},
                .whitePointX = 15635,
                .whitePointY = 16450,
                .maxDisplayMasteringLuminance = 1000,
                .minDisplayMasteringLuminance = 50,
                .maxContentLightLevel = 1000,
                .maxPicAverageLightLevel = 400,
        };
        SS4S_PlayerVideoSetHDRInfo(session_active->player, &info);
    } else {
        SS4S_PlayerVideoSetHDRInfo(session_active->player, NULL);
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

static void streaming_thread_wait(SDL_Thread *thread) {
    SDL_WaitThread(thread, NULL);
}