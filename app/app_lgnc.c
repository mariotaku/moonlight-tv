#include "app.h"
#include "main.h"

#include <lgnc_system.h>

void app_init()
{
    LGNC_SYSTEM_CALLBACKS_T callbacks = {
        .pfnJoystickEventCallback = NULL,
        .pfnMsgHandler = NULL,
        .pfnKeyEventCallback = NULL,
        .pfnMouseEventCallback = NULL};
    if (LGNC_SYSTEM_Initialize(argc, argv, NULL) != 0)
    {
        return -1;
    }
}

void app_destroy()
{
    LGNC_SYSTEM_Finalize();
}