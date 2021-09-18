#include "pi_common.h"
#include "stream/module/api.h"
#include "util/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ilclient.h>
#include <bcm_host.h>

#include <SDL.h>

static void presenter_enter_fullscreen()
{
    OMX_CONFIG_DISPLAYREGIONTYPE displayRegion;
    displayRegion.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    displayRegion.nVersion.nVersion = OMX_VERSION;
    displayRegion.nPortIndex = 90;
    displayRegion.fullscreen = OMX_TRUE;
    displayRegion.set = OMX_DISPLAY_SET_FULLSCREEN;

    if (OMX_SetParameter(ILC_GET_HANDLE(video_render), OMX_IndexConfigDisplayRegion, &displayRegion) != OMX_ErrorNone)
    {
        applog_e("Pi", "Failed to set video region parameters\n");
    }
    
    SDL_ShowCursor(SDL_FALSE);
}

static void presenter_enter_overlay(int x, int y, int w, int h)
{
    OMX_CONFIG_DISPLAYREGIONTYPE displayRegion;
    displayRegion.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    displayRegion.nVersion.nVersion = OMX_VERSION;
    displayRegion.nPortIndex = 90;
    displayRegion.fullscreen = OMX_FALSE;
    displayRegion.dest_rect.x_offset = x;
    displayRegion.dest_rect.y_offset = y;
    displayRegion.dest_rect.width = w;
    displayRegion.dest_rect.height = h;
    displayRegion.set = OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_DEST_RECT;

    if (OMX_SetParameter(ILC_GET_HANDLE(video_render), OMX_IndexConfigDisplayRegion, &displayRegion) != OMX_ErrorNone)
    {
        applog_e("Pi", "Failed to set video region parameters\n");
    }

    SDL_ShowCursor(SDL_TRUE);
}

VIDEO_PRESENTER_CALLBACKS presenter_callbacks_pi = {
    .enterFullScreen = presenter_enter_fullscreen,
    .enterOverlay = presenter_enter_overlay,
};