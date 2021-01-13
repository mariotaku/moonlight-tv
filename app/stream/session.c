#include "session.h"
#include "settings.h"

#include "src/connection.h"
#include "src/platform.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "input/absinput.h"

#include <Limelight.h>
#include <pthread.h>

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include "backend/computer_manager.h"

STREAMING_STATUS streaming_status = STREAMING_NONE;
int streaming_errno = GS_OK;

bool session_running = false;
pthread_t streaming_thread;
pthread_mutex_t lock;
pthread_cond_t cond;

short streaming_display_width, streaming_display_height;
bool streaming_quitapp_requested;

typedef struct
{
    SERVER_LIST *node;
    CONFIGURATION *config;
    int appId;
} STREAMING_REQUEST;

static void *_streaming_thread_action(STREAMING_REQUEST *req);

void streaming_init()
{
    streaming_status = STREAMING_NONE;
    streaming_quitapp_requested = false;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    absinput_init();
}

void streaming_destroy()
{
    streaming_interrupt(streaming_quitapp_requested);
    streaming_wait_for_stop();

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
}

void streaming_begin(PSERVER_LIST node, int app_id)
{
    PCONFIGURATION config = settings_load();

    if (config->stream.bitrate < 0)
    {
        config->stream.bitrate = settings_optimal_bitrate(config->stream.width, config->stream.height, config->stream.fps);
    }

    STREAMING_REQUEST *req = malloc(sizeof(STREAMING_REQUEST));
    req->node = node;
    req->config = config;
    req->appId = app_id;
    pthread_create(&streaming_thread, NULL, (void *(*)(void *))_streaming_thread_action, req);
}

void streaming_interrupt(bool quitapp)
{
    pthread_mutex_lock(&lock);
    streaming_quitapp_requested = quitapp;
    session_running = false;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}

void streaming_wait_for_stop()
{
    pthread_join(streaming_thread, NULL);
}

bool streaming_running()
{
    return session_running;
}

void streaming_display_size(short width, short height)
{
    streaming_display_width = width;
    streaming_display_height = height;
}

void *_streaming_thread_action(STREAMING_REQUEST *req)
{
    streaming_status = STREAMING_CONNECTING;
    streaming_errno = GS_OK;
    PSERVER_LIST node = req->node;
    PSERVER_DATA server = node->server;
    PCONFIGURATION config = req->config;
    absinput_no_control = config->viewonly;
    int appId = req->appId;

    int gamepads = absinput_gamepads();
    int gamepad_mask = 0;
    for (int i = 0; i < gamepads && i < 4; i++)
        gamepad_mask = (gamepad_mask << 1) + 1;

    if (config->debug_level > 0)
    {
        printf("Launch app %d...\n", appId);
    }
    int ret = gs_start_app(server, &config->stream, appId, config->sops, config->localaudio, gamepad_mask);
    if (ret < 0)
    {
        streaming_status = STREAMING_ERROR;
        streaming_errno = ret;
        goto thread_cleanup;
    }

    int drFlags = 0;

    if (config->debug_level > 0)
    {
        printf("Stream %d x %d, %d fps, %d kbps\n", config->stream.width, config->stream.height, config->stream.fps, config->stream.bitrate);
    }

    int startResult = LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks,
                                        platform_get_video(NONE), platform_get_audio(NONE, config->audio_device), NULL, drFlags, config->audio_device, 0);
    if (startResult != 0)
    {
        streaming_status = STREAMING_ERROR;
        streaming_errno = GS_WRONG_STATE;
        goto thread_cleanup;
    }
    session_running = true;
    streaming_status = STREAMING_STREAMING;
    rumble_handler = absinput_getrumble();

    pthread_mutex_lock(&lock);
    while (session_running)
    {
        // Wait until interrupted
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    rumble_handler = NULL;
    streaming_status = STREAMING_DISCONNECTING;
    LiStopConnection();

    if (config->quitappafter || streaming_quitapp_requested)
    {
        if (config->debug_level > 0)
            printf("Sending app quit request ...\n");
        gs_quit_app(server);
    }
    streaming_status = STREAMING_NONE;
    bus_pushevent(USER_CM_REQ_SERVER_UPDATE, node, NULL);

thread_cleanup:
    free(req);
    pthread_exit(NULL);
    return NULL;
}