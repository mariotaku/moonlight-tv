#include "VideoSinkManagerService.h"

#include <string.h>

#include <libhelpers.h>

#include "lunasynccall.h"

static void _hcontext_reset(HContext *hcontext)
{
    memset(hcontext, 0, sizeof(HContext));
    hcontext->callback = LSSyncCallbackObtain();
    hcontext->public = true;
    hcontext->multiple = false;
}

bool VideoSinkManagerRegister(const char *contextId)
{
    LSSyncCallInit();
    HContext hcontext;
    _hcontext_reset(&hcontext);

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\",\"resourceList\":[{\"type\":\"VDEC\",\"portNumber\":%d}]}",
             contextId, 0);
    if (HLunaServiceCall("luna://com.webos.service.videosinkmanager/private/register", buf, &hcontext) != 0)
    {
        LSSyncCallbackUnlock();
        return false;
    }
    printf("[LSCall] %s <= %s\n", "luna://com.webos.service.videosinkmanager/private/register", buf);
    LSMessage *resp = LSWaitForMessage();
    printf("[LSResp] %s => %s\n", LSMessageGetSenderServiceName(resp), LSMessageGetPayload(resp));
    LSMessageUnref(resp);
    return true;
}

bool VideoSinkManagerUnregister(const char *contextId)
{
    LSSyncCallInit();
    HContext hcontext;
    _hcontext_reset(&hcontext);

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\"}",
             contextId);
    if (HLunaServiceCall("luna://com.webos.service.videosinkmanager/private/unregister", buf, &hcontext) != 0)
    {
        LSSyncCallbackUnlock();
        return false;
    }
    printf("[LSCall] %s <= %s\n", "luna://com.webos.service.videosinkmanager/private/unregister", buf);
    LSMessage *resp = LSWaitForMessage();
    printf("[LSResp] %s => %s\n", LSMessageGetSenderServiceName(resp), LSMessageGetPayload(resp));
    LSMessageUnref(resp);
    return true;
}

bool AcbSetMediaVideoData(const char *contextId, int framerate, int width, int height)
{
    LSSyncCallInit();
    HContext hcontext;
    _hcontext_reset(&hcontext);

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\",\"content\":\"movie\",\"video\":{\"scanType\":\"VIDEO_PROGRESSIVE\","
             "\"frameRate\":%d,\"width\":%d,\"height\":%d,"
             "\"pixelAspectRatio\":{\"width\":1,\"height\":1},\"data3D\":"
             "{\"originalPattern\":\"2d\",\"currentPattern\":\"2d\",\"typeLR\":\"LR\"},\"adaptive\":true}}",
             contextId, framerate, width, height);
    if (HLunaServiceCall("luna://com.webos.service.acb/setVideoInfo", buf, &hcontext) != 0)
    {
        LSSyncCallbackUnlock();
        return false;
    }
    printf("[LSCall] %s <= %s\n", "luna://com.webos.service.acb/setVideoInfo", buf);
    LSMessage *resp = LSWaitForMessage();
    printf("[LSResp] %s => %s\n", LSMessageGetSenderServiceName(resp), LSMessageGetPayload(resp));
    LSMessageUnref(resp);
    return true;
}
