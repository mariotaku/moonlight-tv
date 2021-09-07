#include "session.h"
#include "settings.h"
#include "app.h"

#include <stdarg.h>
#include <string.h>

#include "connection.h"
#include "platform.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "input/absinput.h"
#include "video/delegate.h"
#include "backend/pcmanager/priv.h"

#include <Limelight.h>
#include <pthread.h>

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include "backend/pcmanager.h"

#include "util/logging.h"
#include "util/memlog.h"

STREAMING_STATUS streaming_status = STREAMING_NONE;
int streaming_errno = GS_OK;
char streaming_errmsg[1024];

bool streaming_running = false, session_interrupted = false;

short streaming_display_width, streaming_display_height;
bool streaming_quitapp_requested;

pthread_mutex_t streaming_errmsg_lock, streaming_interrupt_lock, streaming_status_lock;
pthread_cond_t streaming_interrupt_cond;
pthread_t streaming_thread;

static PVIDEO_PRESENTER_CALLBACKS video_presenter_cb = NULL;

typedef struct
{
    SERVER_DATA *server;
    CONFIGURATION *config;
    int appId;
} STREAMING_REQUEST;

static void *_streaming_thread_action(STREAMING_REQUEST *req);
static void _streaming_set_status(STREAMING_STATUS status);
static bool _streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps);

void streaming_init()
{
    streaming_status = STREAMING_NONE;
    streaming_quitapp_requested = false;
    session_interrupted = false;
    pthread_mutex_init(&streaming_errmsg_lock, NULL);
    pthread_mutex_init(&streaming_interrupt_lock, NULL);
    pthread_mutex_init(&streaming_status_lock, NULL);
    pthread_cond_init(&streaming_interrupt_cond, NULL);
}

void streaming_destroy()
{
    streaming_interrupt(false);
    streaming_wait_for_stop();

    pthread_cond_destroy(&streaming_interrupt_cond);
    pthread_mutex_destroy(&streaming_interrupt_lock);
    pthread_mutex_destroy(&streaming_status_lock);
    pthread_mutex_destroy(&streaming_errmsg_lock);
}

int streaming_begin(const SERVER_DATA *server, const APP_LIST *app)
{
    if (streaming_status != STREAMING_NONE)
    {
        return -1;
    }
    PCONFIGURATION config = settings_load();

    if (config->stream.bitrate < 0)
        config->stream.bitrate = settings_optimal_bitrate(config->stream.width, config->stream.height, config->stream.fps);
    // Cap framerate to platform request
    if (decoder_info.maxBitrate && config->stream.bitrate > decoder_info.maxBitrate)
        config->stream.bitrate = decoder_info.maxBitrate;
    config->sops &= _streaming_sops_supported(server->modes, config->stream.width, config->stream.height, config->stream.fps);
    config->stream.supportsHevc = decoder_info.hevc;
    config->stream.enableHdr &= decoder_info.hevc && decoder_info.hdr && server->supportsHdr &&
                                (decoder_info.hdr == DECODER_HDR_ALWAYS || app->hdr != 0);
    config->stream.colorSpace = decoder_info.colorSpace;
    if (config->stream.enableHdr)
        config->stream.colorRange = decoder_info.colorRange;
#if HAVE_SURROUND_SOUND
    if (!config->stream.audioConfiguration)
        config->stream.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
    else if (CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(module_audio_configuration()) < CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config->stream.audioConfiguration))
        config->stream.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
#endif
    config->stream.encryptionFlags = ENCFLG_AUDIO;

    STREAMING_REQUEST *req = malloc(sizeof(STREAMING_REQUEST));
    req->server = serverdata_new();
    memcpy(req->server, server, sizeof(SERVER_DATA));
    req->config = config;
    req->appId = app->id;
    return pthread_create(&streaming_thread, NULL, (void *(*)(void *))_streaming_thread_action, req);
}

void streaming_interrupt(bool quitapp)
{
    if (session_interrupted)
    {
        return;
    }
    pthread_mutex_lock(&streaming_interrupt_lock);
    streaming_quitapp_requested = quitapp;
    session_interrupted = true;
    pthread_cond_signal(&streaming_interrupt_cond);
    pthread_mutex_unlock(&streaming_interrupt_lock);
}

void streaming_wait_for_stop()
{
    if (streaming_status != STREAMING_NONE)
    {
        pthread_join(streaming_thread, NULL);
    }
}

void streaming_display_size(short width, short height)
{
    streaming_display_width = width;
    streaming_display_height = height;
}

void *_streaming_thread_action(STREAMING_REQUEST *req)
{
    pthread_mutex_lock(&streaming_interrupt_lock);
    session_interrupted = false;
    pthread_mutex_unlock(&streaming_interrupt_lock);
#if _GNU_SOURCE
    pthread_setname_np(pthread_self(), "session");
#endif
    _streaming_set_status(STREAMING_CONNECTING);
    streaming_errno = GS_OK;

    _streaming_errmsg_write("");
    PSERVER_DATA server = req->server;
    PCONFIGURATION config = req->config;
    absinput_no_control = config->viewonly;
    int appId = req->appId;

    int gamepads = absinput_gamepads();
    int gamepad_mask = 0;
    for (int i = 0; i < gamepads && i < 4; i++)
        gamepad_mask = (gamepad_mask << 1) + 1;

    applog_i("Session", "Launch app %d...", appId);
    GS_CLIENT client = app_gs_client_obtain();
    int ret = gs_start_app(client, server, &config->stream, appId, config->sops, config->localaudio, gamepad_mask);
    if (ret < 0)
    {
        _streaming_set_status(STREAMING_ERROR);
        streaming_errno = ret;
        applog_e("Session", "Failed to launch session: gamestream returned %d", ret);
        goto thread_cleanup;
    }

    int drFlags = 0;

    applog_i("Session", "Video %d x %d, %d fps, %d kbps", config->stream.width, config->stream.height, config->stream.fps, config->stream.bitrate);
    applog_i("Session", "Audio %d channels", CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config->stream.audioConfiguration));

    PDECODER_RENDERER_CALLBACKS vdec = decoder_get_video();
    PAUDIO_RENDERER_CALLBACKS adec = module_get_audio(config->audio_device);
    PVIDEO_PRESENTER_CALLBACKS pres = decoder_get_presenter();
    DECODER_RENDERER_CALLBACKS vdec_delegate = decoder_render_callbacks_delegate(vdec);

    int startResult = LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks,
                                        &vdec_delegate, adec, vdec, drFlags, (void *)config->audio_device, 0);
    if (startResult != 0)
    {
        _streaming_set_status(STREAMING_ERROR);
        streaming_errno = GS_WRONG_STATE;
        applog_e("Session", "Failed to start connection: Limelight returned %d", startResult);
        goto thread_cleanup;
    }
    streaming_running = true;
    _streaming_set_status(STREAMING_STREAMING);
    video_presenter_cb = pres;
    bus_pushevent(USER_STREAM_OPEN, &config->stream, NULL);
    pthread_mutex_lock(&streaming_interrupt_lock);
    while (!session_interrupted)
    {
        // Wait until interrupted
        pthread_cond_wait(&streaming_interrupt_cond, &streaming_interrupt_lock);
    }
    video_presenter_cb = NULL;
    session_interrupted = false;
    pthread_mutex_unlock(&streaming_interrupt_lock);
    streaming_running = false;
    bus_pushevent(USER_STREAM_CLOSE, NULL, NULL);

    _streaming_set_status(STREAMING_DISCONNECTING);
    LiStopConnection();

    if (streaming_quitapp_requested)
    {
        applog_i("Session", "Sending app quit request ...");
        gs_quit_app(client, server);
    }
    pcmanager_request_update(server);

    // Don't always reset status as error state should be kept
    _streaming_set_status(STREAMING_NONE);
thread_cleanup:
    bus_pushevent(USER_STREAM_FINISHED, NULL, NULL);
    free(req->server);
    free(req->config);
    free(req);
    return NULL;
}

void streaming_enter_fullscreen()
{
    app_set_mouse_grab(!app_configuration->absmouse);
    if (video_presenter_cb)
        video_presenter_cb->enterFullScreen();
}

void streaming_enter_overlay(int x, int y, int w, int h)
{
    app_set_mouse_grab(false);
    if (video_presenter_cb)
        video_presenter_cb->enterOverlay(x, y, w, h);
}

void _streaming_set_status(STREAMING_STATUS status)
{
    pthread_mutex_lock(&streaming_status_lock);
    streaming_status = status;
    pthread_mutex_unlock(&streaming_status_lock);
}

void _streaming_errmsg_write(const char *fmt, ...)
{
    pthread_mutex_lock(&streaming_errmsg_lock);
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(streaming_errmsg, sizeof(streaming_errmsg) / sizeof(char), fmt, arglist);
    va_end(arglist);
    pthread_mutex_unlock(&streaming_errmsg_lock);
}

bool _streaming_sops_supported(PDISPLAY_MODE modes, int w, int h, int fps)
{
    for (PDISPLAY_MODE cur = modes; cur != NULL; cur = cur->next)
    {
        if (cur->width == w && cur->height == h && cur->refresh == fps)
            return true;
    }
    return false;
}