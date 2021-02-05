#include "app_init.h"
#include "app.h"
#include "stream/settings.h"

#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <SDL_syswm.h>
#include "platform/webos/SDL_webOS.h"

#include <NDL_directmedia.h>
#include <lgnc_system.h>

bool app_webos_ndl = false;
bool app_webos_lgnc = false;

static struct wl_seat *_seat = NULL;

static void _registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void _registry_remover(void *data, struct wl_registry *registry, uint32_t id);
static void _seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps);

static const struct wl_registry_listener _registery_listener = {
    _registry_handler,
    _registry_remover,
};
static const struct wl_seat_listener _seat_listener = {
    _seat_handle_capabilities,
};

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
    SDL_SysWMinfo info;
    if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_WAYLAND)
    {
        return;
    }

    struct wl_display *display = info.info.wl.display;
    struct wl_registry *registery = wl_display_get_registry(display);
    wl_registry_add_listener(registery, &_registery_listener, NULL);

    wl_display_dispatch(display);
    // wait for a synchronous response
    wl_display_roundtrip(display);
}

void _registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_seat") == 0)
    {
        _seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(_seat, &_seat_listener, NULL);
    }
}

void _registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
}

void _seat_handle_capabilities(void *data, struct wl_seat *seat,
                               enum wl_seat_capability caps)
{

}