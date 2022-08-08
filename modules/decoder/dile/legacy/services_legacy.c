#include "media_services.h"

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "utils.h"

#include "VideoSinkManagerService.h"
#include "TVService.h"
#include <AcbAPI.h>

static void AcbAPICallback(long acbId, long taskId, long eventType, long appState, long playState, const char *reply);

struct MEDIA_SERVICES_CONTEXT
{
    long acbId;
};

MEDIA_SERVICES_HANDLE media_services_connect(const char *connId, const char *appId, jvalue_ref resources, MEDIA_SERVICES_TYPE type)
{
    jvalue_ref reslist = jobject_get(resources, J_CSTR_TO_BUF("resources"));
    int port = find_source_port(reslist, type == MEDIA_SERVICES_TYPE_VIDEO ? "VDEC" : "PCMMC");
    if (type == MEDIA_SERVICES_TYPE_VIDEO)
        VideoSinkManagerRegisterVDEC(connId, port);
    else
        VideoSinkManagerRegisterPCMMC(connId, port, "game_directmedia");

    long acbId = AcbAPI_create();
    if (acbId < 0)
    {
        printf("AcbAPI_create returned %d\n", acbId);
        return NULL;
    }
    if (!AcbAPI_initialize(acbId, PLAYER_TYPE_VIDEO, appId, AcbAPICallback))
    {
        printf("AcbAPI_initialize returned false\n");
        return NULL;
    }
    if (!AcbAPI_setSinkType(acbId, SINK_TYPE_AUTO))
    {
        printf("AcbAPI_setSinkType returned false\n");
        return NULL;
    }
    if (!AcbAPI_setMediaId(acbId, connId))
    {
        printf("AcbAPI_setMediaId returned false\n");
        return NULL;
    }
    if (AcbAPI_setState(acbId, APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return NULL;
    }
    MEDIA_SERVICES_HANDLE hnd = malloc(sizeof(struct MEDIA_SERVICES_CONTEXT));
    hnd->acbId = acbId;
    return hnd;
}

bool media_services_disconnect(MEDIA_SERVICES_HANDLE hnd, const char *connId)
{
    long acbId = hnd->acbId;
    AcbAPI_setState(acbId, APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED, NULL);

    AcbAPI_finalize(acbId);

    AcbAPI_destroy(acbId);

    VideoSinkManagerUnregister(connId);
    free(hnd);
    return true;
}

bool media_services_set_video_data(MEDIA_SERVICES_HANDLE hnd, const char *contextId, int framerate, int width, int height)
{
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\",\"content\":\"movie\",\"video\":{\"scanType\":\"VIDEO_PROGRESSIVE\","
             "\"frameRate\":%d,\"width\":%d,\"height\":%d,"
             "\"pixelAspectRatio\":{\"width\":1,\"height\":1},\"data3D\":"
             "{\"originalPattern\":\"2d\",\"currentPattern\":\"2d\",\"typeLR\":\"LR\"},\"adaptive\":true}}",
             contextId, framerate, width, height);
    if (AcbAPI_setMediaVideoData(hnd->acbId, buf) < 0)
    {
        printf("[ACB] Failed to set media video data\n");
        return false;
    }
    if (AcbAPI_setDisplayWindow(hnd->acbId, 0, 0, width, height, true, NULL) < 0)
    {
        printf("[ACB] Failed set video display window\n");
        return false;
    }

    TVService_SetLowDelayMode(true);

    if (AcbAPI_setState(hnd->acbId, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return false;
    }
    return true;
}

bool media_services_set_audio_data(MEDIA_SERVICES_HANDLE hnd, const char *connId)
{
    if (AcbAPI_setState(hnd->acbId, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return false;
    }
    return true;
}

bool media_services_feed_arrived(MEDIA_SERVICES_HANDLE hnd)
{
    if (AcbAPI_setState(hnd->acbId, APPSTATE_FOREGROUND, PLAYSTATE_SEAMLESS_LOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return false;
    }
    return true;
}

bool media_services_supported()
{
    // True if you can link to libAcbAPI
    return true;
}

static void AcbAPICallback(long acbId, long taskId, long eventType, long appState, long playState, const char *reply)
{
    printf("AcbAPICallback acbId = %ld, taskId = %ld, eventType = %ld, appState = %ld,playState = %ld, reply = %s EOL\n",
           acbId, taskId, eventType, appState, playState, reply);
}