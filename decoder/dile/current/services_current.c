#include "vdec_services.h"
#include "utils.h"

#include "VideoOutputService.h"

bool DECODER_SYMBOL_NAME(vdec_services_connect)(const char *connId, const char *appId, jvalue_ref resources)
{
    VideoOutputRegister(connId, appId);
    jvalue_ref reslist = jobject_get(resources, J_CSTR_TO_BUF("resources"));
    VideoOutputConnect(connId, find_source_port(reslist));
    return true;
}

bool DECODER_SYMBOL_NAME(vdec_services_disconnect)(const char *connId)
{
    VideoOutputDisconnect(connId);

    VideoOutputUnregister(connId);
    return true;
}

bool DECODER_SYMBOL_NAME(vdec_services_set_data)(const char *connId, int framerate, int width, int height)
{
    VideoOutputSetVideoData(connId, framerate, width, height);

    VideoOutputSetDisplayWindow(connId, true, 0, 0, width, height);
    return true;
}

bool DECODER_SYMBOL_NAME(vdec_services_supported)()
{
    return VideoOutputGetStatus();
}