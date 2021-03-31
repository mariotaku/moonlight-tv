#include "app_init.h"
#include "app.h"
#include "stream/settings.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform/webos/SDL_webOS.h"

void app_webos_init(int argc, char *argv[])
{
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT, "true");
    SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
    // SDL_SetHint(SDL_HINT_WEBOS_REGISTER_APP, "1");
}

void app_webos_destroy()
{
}

void app_webos_window_setup(SDL_Window *window)
{
    int refresh_rate, panel_width, panel_height;
    SDL_webOSGetRefreshRate(&refresh_rate);
    SDL_webOSGetPanelResolution(&panel_width, &panel_height);
    printf("webOS TV: refresh: %d Hz, panel: %d * %d\n", refresh_rate, panel_width, panel_height);
    // const char *ew = SDL_webOSCreateExportedWindow(0);
    // SDL_Rect src = {0, 0, 1920, 1088};
    // SDL_Rect dst = {0, 0, 1920, 1080};
    // SDL_Rect crop = {0, 0, 100, 100};
    // SDL_webOSSetExportedWindow(ew, &src, &dst);
    // SDL_webOSExportedSetCropRegion(ew, &src, &dst, &crop);
    // SDL_webOSDestroyExportedWindow(ew);
}