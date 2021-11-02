#include "TVService.h"

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

bool TVService_SetLowDelayMode(bool value)
{
    LSSyncCallInit();
    HContext hcontext;
    _hcontext_reset(&hcontext);

    char buf[32];
    snprintf(buf, sizeof(buf), "{\"lowDelayMode\":%s}", value ? "true" : "false");
    if (HLunaServiceCall("luna://com.webos.service.tv.display/setLowDelayMode", buf, &hcontext) != 0)
    {
        LSSyncCallbackUnlock();
        return false;
    }
    printf("[LSCall] %s <= %s\n", "luna://com.webos.service.tv.display/setLowDelayMode", buf);
    LSMessage *resp = LSWaitForMessage();
    printf("[LSResp] %s => %s\n", LSMessageGetSenderServiceName(resp), LSMessageGetPayload(resp));
    LSMessageUnref(resp);
    return true;
}