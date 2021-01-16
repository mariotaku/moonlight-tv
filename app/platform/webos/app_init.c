#include "app_init.h"
#include "stream/settings.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <NDL_directmedia.h>
#include <lgnc_system.h>

bool app_webos_ndl = false;
bool app_webos_lgnc = false;

static PCONFIGURATION config;

int app_webos_init(int argc, char *argv[])
{
    config = settings_load();
    // Try NDL if not forced to legacy
    if (strcmp("legacy", config->platform))
    {
        if (NDL_DirectMediaInit(WEBOS_APPID, NULL) == 0)
        {
            app_webos_ndl = true;
            return 0;
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
        return 0;
    }
    else
    {
        fprintf(stderr, "Unable to initialize LGNC\n");
    }

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
    free(config);
}