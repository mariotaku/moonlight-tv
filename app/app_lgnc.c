#include <unistd.h>

#include "app.h"
#include "main.h"

#include "platform/lgnc/callbacks.h"
#include "platform/lgnc/graphics.h"

#include <lgnc_system.h>

#define NK_IMPLEMENTATION
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_widgets.h"
#include "nuklear/ext_functions.h"

#define NK_LGNC_GLES2_IMPLEMENTATION
#include "nuklear/platform_lgnc_gles2.h"

#include "backend/backend_root.h"
#include "stream/input/absinput.h"
#include "ui/gui_root.h"
#include "ui/config.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

int app_init(int argc, char *argv[])
{
    LGNC_SYSTEM_CALLBACKS_T callbacks = {
        .pfnJoystickEventCallback = NULL,
        .pfnMsgHandler = _MsgEventHandler,
        .pfnKeyEventCallback = _KeyEventCallback,
        .pfnMouseEventCallback = _MouseEventCallback};
    if (LGNC_SYSTEM_Initialize(argc, argv, &callbacks) != 0)
    {
        return -1;
    }
    fprintf(stderr, "LGNC_SYSTEM_Initialize finished with 0\n");
    return 0;
}

void app_destroy()
{
    LGNC_SYSTEM_Finalize();
}

APP_WINDOW_CONTEXT app_window_create()
{
    nk_platform_gl_setup();

    int displayId;
    if (LGNC_SYSTEM_GetDisplayId(&displayId) != 0)
    {
        fprintf(stderr, "LGNC_SYSTEM_GetDisplayId failed\n");
    }

    open_display(1280, 720, displayId);
    return NULL;
}

void app_main_loop(void *data)
{
    struct nk_context *ctx = (struct nk_context *)data;

    bool cont = gui_root(ctx);

    /* Draw */
    {
        gui_background();
        /* 
         * IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
         * with blending, scissor, face culling, depth test and viewport and
         * defaults everything back into a default state.
         * Make sure to either a.) save and restore or b.) reset your own state after
         * rendering the UI.
         */

        nk_lgnc_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        gfx_commit();
    }
    if (cont)
    {
        usleep(16 * 1000);
    }
    else
    {
        request_exit();
    }
}