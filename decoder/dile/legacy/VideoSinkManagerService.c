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

static bool VideoSinkManagerRegister(const char *contextId, const char *payload)
{
    LSSyncCallInit();
    HContext hcontext;
    _hcontext_reset(&hcontext);

    if (HLunaServiceCall("luna://com.webos.service.videosinkmanager/private/register", payload, &hcontext) != 0)
    {
        LSSyncCallbackUnlock();
        return false;
    }
    printf("[LSCall] %s <= %s\n", "luna://com.webos.service.videosinkmanager/private/register", payload);
    LSMessage *resp = LSWaitForMessage();
    printf("[LSResp] %s => %s\n", LSMessageGetSenderServiceName(resp), LSMessageGetPayload(resp));
    LSMessageUnref(resp);
    return true;
}

bool VideoSinkManagerRegisterVDEC(const char *contextId)
{
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\",\"resourceList\":[{\"type\":\"VDEC\",\"portNumber\":%d}]}",
             contextId, 0);
    return VideoSinkManagerRegister(contextId, buf);
}

bool VideoSinkManagerRegisterPCMMC(const char *contextId, const char *audioType)
{
    char buf[1024], atyp[128];
    if (audioType)
        snprintf(atyp, sizeof(atyp), "\"audioType\":\"%s\"", audioType);
    else
        atyp[0] = '\0';
    snprintf(buf, sizeof(buf),
             "{\"context\":\"%s\",\"resourceList\":[{\"type\":\"VDEC\",\"portNumber\":%d}]%s}",
             contextId, 0, atyp);
    return VideoSinkManagerRegister(contextId, buf);
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

bool VideoSinkManagerGetConnectionState()
{
    LSSyncCallInit();
    HContext hcontext;
    _hcontext_reset(&hcontext);

    if (HLunaServiceCall("luna://com.webos.service.videosinkmanager/getConnectionState", "{}", &hcontext) != 0)
    {
        LSSyncCallbackUnlock();
        return false;
    }
    printf("[LSCall] %s <= %s\n", "luna://com.webos.service.videosinkmanager/getConnectionState", "{}");
    LSMessage *resp = LSWaitForMessage();
    printf("[LSResp] %s => %s\n", LSMessageGetSenderServiceName(resp), LSMessageGetPayload(resp));
    LSMessageUnref(resp);
    return true;
}