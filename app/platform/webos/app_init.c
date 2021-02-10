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

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <SDL_syswm.h>

#include <NDL_directmedia.h>
#include <lgnc_system.h>

bool app_webos_ndl = false;
bool app_webos_lgnc = false;

static struct wl_seat *_seat = NULL;
static struct wl_keyboard *_keyboard = NULL;
static struct xkb_context *_xkb = NULL;
static struct xkb_keymap *_xkb_keymap = NULL;
static struct xkb_state *_xkb_state = NULL;

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

    _xkb = xkb_context_new(0);
    return 0;
}

void app_webos_destroy()
{
    xkb_context_unref(_xkb);
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

void app_webos_handle_keyevent(SDL_KeyboardEvent *event)
{
    printf("xkb: %d mapped to ", event->keysym.sym);
    const xkb_keysym_t *out = NULL;
    xkb_state_key_get_syms(_xkb_state, event->keysym.sym, &out);
    if (out)
    {
        printf("%d\n", *out);
    }
    else
    {
        printf("nothing\n");
    }
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

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
    char *map_str;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        close(fd);
        return;
    }

    map_str = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_str == MAP_FAILED)
    {
        close(fd);
        return;
    }

    _xkb_keymap = xkb_keymap_new_from_string(_xkb, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    munmap(map_str, size);
    close(fd);

    if (!_xkb_keymap)
    {
        fprintf(stderr, "failed to compile keymap\n");
        return;
    }

    _xkb_state = xkb_state_new(_xkb_keymap);
    if (!_xkb_state)
    {
        fprintf(stderr, "failed to create XKB state\n");
        xkb_keymap_unref(_xkb_keymap);
        _xkb_keymap = NULL;
        return;
    }
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
    printf("keyboard_handle_enter\n");
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
    printf("keyboard_handle_leave\n");
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
    printf("keyboard_handle_key\n");
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
    printf("keyboard_handle_modifiers\n");
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
};

void _seat_handle_capabilities(void *data, struct wl_seat *seat,
                               enum wl_seat_capability caps)
{
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !_keyboard)
    {
        _keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_set_user_data(_keyboard, NULL);
        wl_keyboard_add_listener(_keyboard, &keyboard_listener, NULL);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && _keyboard)
    {
        wl_keyboard_destroy(_keyboard);
        _keyboard = NULL;
    }
}