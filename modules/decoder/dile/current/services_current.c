#include "media_services.h"
#include "utils.h"

#include "VideoOutputService.h"

struct MEDIA_SERVICES_CONTEXT
{
    int placeholder;
};

MEDIA_SERVICES_HANDLE media_services_connect(const char *connId, const char *appId, jvalue_ref resources, MEDIA_SERVICES_TYPE type)
{
    jvalue_ref reslist = jobject_get(resources, J_CSTR_TO_BUF("resources"));
    int port = find_source_port(reslist, type == MEDIA_SERVICES_TYPE_VIDEO ? "VDEC" : "ADEC");
    VideoOutputRegister(connId, appId);
    VideoOutputConnect(connId, port);
    MEDIA_SERVICES_HANDLE hnd = malloc(sizeof(struct MEDIA_SERVICES_CONTEXT));
    hnd->placeholder = 0;
    return hnd;
}

bool media_services_disconnect(MEDIA_SERVICES_HANDLE hnd, const char *connId)
{
    VideoOutputDisconnect(connId);

    VideoOutputUnregister(connId);
    free(hnd);
    return true;
}

bool media_services_set_video_data(MEDIA_SERVICES_HANDLE hnd, const char *connId, int framerate, int width, int height)
{
    VideoOutputSetVideoData(connId, framerate, width, height);

    VideoOutputSetDisplayWindow(connId, true, 0, 0, width, height);
    return true;
}

bool media_services_set_audio_data(MEDIA_SERVICES_HANDLE hnd, const char *connId)
{
    return true;
}

bool media_services_feed_arrived(MEDIA_SERVICES_HANDLE hnd)
{
    return true;
}

bool media_services_supported()
{
    return VideoOutputGetStatus();
}