#include "app_init.h"
#include "app.h"
#include "stream/settings.h"

#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL_syswm.h>
#include "platform/webos/SDL_webOS.h"

#include <NDL_directmedia.h>
#include <lgnc_system.h>

bool app_webos_ndl = false;
bool app_webos_lgnc = false;

int app_webos_init(int argc, char *argv[])
{
    // Try NDL if not forced to legacy
    if (strcmp("legacy", app_configuration->platform))
    {
        if (NDL_DirectMediaInit(WEBOS_APPID, NULL) == 0)
        {
            app_webos_ndl = true;
            goto finish;
        }
        else
        {
            fprintf(stderr, "Unable to initialize NDL: %s\n", NDL_DirectMediaGetError());
        }
    }
    LGNC_SYSTEM_CALLBACKS_T callbacks = {
        .pfnJoystickEventCallback = NULL,
        .pfnMsgHandler = NULL,
        .pfnKeyEventCallback = NULL,
        .pfnMouseEventCallback = NULL};
    if (LGNC_SYSTEM_Initialize(argc, argv, &callbacks) == 0)
    {
        app_webos_lgnc = true;
        goto finish;
    }
    else
    {
        fprintf(stderr, "Unable to initialize LGNC\n");
    }
finish:
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
    return 0;
}

void app_webos_destroy()
{
    if (app_webos_ndl)
    {
        NDL_DirectMediaQuit();
    }
    if (app_webos_lgnc)
    {
        LGNC_SYSTEM_Finalize();
    }
}

void app_webos_window_setup(SDL_Window *window)
{
}