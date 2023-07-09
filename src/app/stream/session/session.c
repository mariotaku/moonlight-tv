#include "app.h"
#include "config.h"
#include "priv.h"

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
#include "callbacks.h"
#include "ss4s.h"
#include "backend/pcmanager/worker/worker.h"
#include "input/input_gamepad.h"
#include "app_session.h"
#include "session_worker.h"
#include "stream/input/input_vmouse.h"

int streaming_errno = GS_OK;
char streaming_errmsg[1024];

static int streaming_worker(session_t *session);

static bool streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps);

static void session_config_init(session_config_t *config, const SERVER_DATA *server, const CONFIGURATION *app_config);

session_t *session_create(app_t *app, const CONFIGURATION *config, const SERVER_DATA *server, int app_id) {
    session_t *session = malloc(sizeof(session_t));
    SDL_memset(session, 0, sizeof(session_t));
    session_config_init(&session->config, server, config);
    session->app = app;
    session->display_width = app->ui.width;
    session->display_height = app->ui.height;
    session->video_cap = app->ss4s.video_cap;
    session->server = serverdata_clone(server);
    // The flags seem to be the same to supportedVideoFormats, use it for now...
    session->server->serverInfo.serverCodecModeSupport = session->config.stream.supportedVideoFormats;
    session->app_id = app_id;
    session->mutex = SDL_CreateMutex();
    session->state_lock = SDL_CreateMutex();
    session->cond = SDL_CreateCond();
    session->input.session = session;
    session->input.input = &app->input;
    session->input.view_only = session->config.view_only;
    session->input.no_sdl_mouse = session->config.hardware_mouse;
    session->thread = SDL_CreateThread((SDL_ThreadFunction) session_worker, "session", session);
#if FEATURE_INPUT_EVMOUSE
    if (!config->viewonly && config->hardware_mouse) {
        session_evmouse_init(session);
    }
#endif
    return session;
}

void session_destroy(session_t *session) {
    serverdata_free(session->server);
    SDL_DestroyCond(session->cond);
    SDL_DestroyMutex(session->mutex);
}

void session_interrupt(session_t *session, bool quitapp, streaming_interrupt_reason_t reason) {
    if (!session || session->interrupted) {
        return;
    }
    SDL_LockMutex(session->mutex);
#if FEATURE_INPUT_EVMOUSE
    session_evmouse_interrupt(session);
#endif
    session->quitapp = quitapp;
    session->interrupted = true;
    if (reason >= STREAMING_INTERRUPT_ERROR) {
        switch (reason) {
            case STREAMING_INTERRUPT_WATCHDOG:
                streaming_error(session, reason, "Stream stalled");
                break;
            case STREAMING_INTERRUPT_NETWORK:
                streaming_error(session, reason, "Network error happened");
                break;
            case STREAMING_INTERRUPT_DECODER:
                streaming_error(session, reason, "Decoder reported error");
                break;
            default:
                streaming_error(session, reason, "Error occurred while in streaming");
                break;
        }
    }
    SDL_CondSignal(session->cond);
    SDL_UnlockMutex(session->mutex);
    SDL_UnlockMutex(session->state_lock);
}

bool session_accepting_input(session_t *session) {
    return session->input.started;
}

void session_start_input(session_t *session) {
    session->input.started = true;
}

void session_stop_input(session_t *session) {
    session->input.started = false;
}

void session_toggle_vmouse(session_t *session) {
    bool value = session->config.vmouse && !session_input_is_vmouse_active(&session->input.vmouse);
    session_input_set_vmouse_active(&session->input.vmouse, value);
}

void streaming_display_size(session_t *session, short width, short height) {
    session->display_width = width;
    session->display_height = height;
}

void streaming_enter_fullscreen(session_t *session) {
    app_set_mouse_grab(&session->app->input, true);
    if ((session->video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_UI_COMPOSITING) == 0) {
        SS4S_PlayerVideoSetDisplayArea(session->player, NULL, NULL);
    }
}

void streaming_enter_overlay(session_t *session, int x, int y, int w, int h) {
    app_set_mouse_grab(&session->app->input, false);
    SS4S_VideoRect dst = {x, y, w, h};
    if ((session->video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_UI_COMPOSITING) == 0) {
        SS4S_PlayerVideoSetDisplayArea(session->player, NULL, &dst);
    }
}

void streaming_set_hdr(session_t *session, bool hdr) {
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
        SS4S_PlayerVideoSetHDRInfo(session->player, &info);
    } else {
        SS4S_PlayerVideoSetHDRInfo(session->player, NULL);
    }
}

void streaming_error(session_t *session, int code, const char *fmt, ...) {
    SDL_LockMutex(session->state_lock);
    streaming_errno = code;
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(streaming_errmsg, sizeof(streaming_errmsg) / sizeof(char), fmt, arglist);
    va_end(arglist);
    SDL_UnlockMutex(session->state_lock);
}

bool streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps) {
    for (PDISPLAY_MODE cur = modes; cur != NULL; cur = cur->next) {
        if (cur->width == w && cur->height == h && cur->refresh == fps) {
            return true;
        }
    }
    return false;
}

void session_config_init(session_config_t *config, const SERVER_DATA *server, const CONFIGURATION *app_config) {
    config->stream = app_config->stream;
    config->vmouse = app_config->virtual_mouse;
    config->hardware_mouse = app_config->hardware_mouse;
    config->local_audio = app_config->localaudio;
    config->view_only = app_config->viewonly;
    config->sops = app_config->sops;

    SS4S_VideoCapabilities video_cap = global->ss4s.video_cap;
    SS4S_AudioCapabilities audio_cap = global->ss4s.audio_cap;

    if (config->stream.bitrate < 0) {
        config->stream.bitrate = settings_optimal_bitrate(&video_cap, config->stream.width, config->stream.height,
                                                          config->stream.fps);
    }
    // Cap framerate to platform request
    if (video_cap.maxBitrate && config->stream.bitrate > video_cap.maxBitrate) {
        config->stream.bitrate = (int) video_cap.maxBitrate;
    }
    config->sops &= streaming_sops_supported(server->modes, config->stream.width, config->stream.height,
                                             config->stream.fps);
    if (video_cap.codecs & SS4S_VIDEO_H264) {
        config->stream.supportedVideoFormats |= VIDEO_FORMAT_H264;
    }
    if (app_config->hevc && video_cap.codecs & SS4S_VIDEO_H265) {
        config->stream.supportedVideoFormats |= VIDEO_FORMAT_H265;
        if (app_config->hdr && video_cap.hdr) {
            config->stream.supportedVideoFormats |= VIDEO_FORMAT_H265_MAIN10;
        }
    }
    config->stream.colorSpace = COLORSPACE_REC_709/* TODO: get from video capabilities */;
    config->stream.colorRange = video_cap.fullColorRange ? COLOR_RANGE_FULL : COLOR_RANGE_LIMITED;
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
}

