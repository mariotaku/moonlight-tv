#define _GNU_SOURCE
#include "vdec_services.h"

#include <stdio.h>
#include <dlfcn.h>

#include "utils.h"

#include "VideoSinkManagerService.h"
#include "TVService.h"
#include "AcbService.h"
#include <AcbAPI.h>

static long acbId;

static void AcbAPICallback(long acbId, long taskId, long eventType, long appState, long playState, const char *reply);

bool vdec_services_connect(const char *connId, const char *appId, jvalue_ref resources)
{
    VideoSinkManagerRegister(connId);

    acbId = AcbAPI_create();
    if (acbId < 0)
    {
        printf("AcbAPI_create returned %d\n", acbId);
        return false;
    }
    if (!AcbAPI_initialize(acbId, PLAYER_TYPE_VIDEO, appId, AcbAPICallback))
    {
        printf("AcbAPI_initialize returned false\n");
        return false;
    }
    if (!AcbAPI_setSinkType(acbId, SINK_TYPE_AUTO))
    {
        printf("AcbAPI_setSinkType returned false\n");
        return false;
    }
    if (!AcbAPI_setMediaId(acbId, connId))
    {
        printf("AcbAPI_setMediaId returned false\n");
        return false;
    }
    if (AcbAPI_setState(acbId, APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return false;
    }
    return true;
}

bool vdec_services_disconnect(const char *connId)
{
    AcbAPI_setState(acbId, APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED, NULL);

    AcbAPI_finalize(acbId);

    AcbAPI_destroy(acbId);

    VideoSinkManagerUnregister(connId);
    return true;
}

bool vdec_services_set_data(const char *contextId, int framerate, int width, int height)
{
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\",\"content\":\"movie\",\"video\":{\"scanType\":\"VIDEO_PROGRESSIVE\","
             "\"frameRate\":%d,\"width\":%d,\"height\":%d,"
             "\"pixelAspectRatio\":{\"width\":1,\"height\":1},\"data3D\":"
             "{\"originalPattern\":\"2d\",\"currentPattern\":\"2d\",\"typeLR\":\"LR\"},\"adaptive\":true}}",
             contextId, framerate, width, height);
    if (AcbAPI_setMediaVideoData(acbId, buf) < 0)
    {
        printf("[ACB] Failed to set media video data\n");
        return false;
    }
    if (AcbAPI_setDisplayWindow(acbId, 0, 0, width, height, true, NULL) < 0)
    {
        printf("[ACB] Failed set video display window\n");
        return false;
    }

    TVService_SetLowDelayMode(true);

    if (AcbAPI_setState(acbId, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return false;
    }
    return true;
}

bool vdec_services_video_arrived()
{
    if (AcbAPI_setState(acbId, APPSTATE_FOREGROUND, PLAYSTATE_SEAMLESS_LOADED, NULL) < 0)
    {
        printf("[ACB] Failed to set state\n");
        return false;
    }
    return true;
}

bool vdec_services_supported()
{
    // True if you you can link to libAcbAPI
    return true;
}

static void AcbAPICallback(long acbId, long taskId, long eventType, long appState, long playState, const char *reply)
{
    printf("AcbAPICallback acbId = %ld, taskId = %ld, eventType = %ld, appState = %ld,playState = %ld, reply = %s EOL\n",
           acbId, taskId, eventType, appState, playState, reply);
}