#include <stdbool.h>
#include <SDL.h>
#include "lvgl/lv_sdl_drv_input.h"
#include "lvgl/lv_disp_drv_app.h"
#include "lvgl/util/lv_app_utils.h"

#include "app.h"
#include "config.h"

#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/root.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "util/logging.h"
#include "util/i18n.h"

PCONFIGURATION app_configuration = NULL;

static bool window_focus_gained;
static SDL_Cursor *blank_cursor = NULL;

static void quit_confirm_cb(lv_event_t *e);

int app_init(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    app_configuration = settings_load();
#if TARGET_WEBOS
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT, "true");
    SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
#endif
    return 0;
}

void app_init_video() {
    SDL_Init(SDL_INIT_VIDEO);
    // This will occupy SDL_USEREVENT
    SDL_RegisterEvents(1);
    SDL_Surface *surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    blank_cursor = SDL_CreateColorCursor(surface, 0, 0);
    if (!blank_cursor) {
        applog_w("Input", "Failed to create blank cursor: %s", SDL_GetError());
    }
}

void inputmgr_sdl_handle_event(SDL_Event *ev);

static int app_event_filter(void *userdata, SDL_Event *event) {
    switch (event->type) {
        case SDL_APP_WILLENTERBACKGROUND: {
            // Interrupt streaming because app will go to background
            streaming_interrupt(false, STREAMING_INTERRUPT_BACKGROUND);
            break;
        }
        case SDL_APP_DIDENTERFOREGROUND: {
            lv_obj_invalidate(lv_scr_act());
            break;
        }
        case SDL_WINDOWEVENT: {
            switch (event->window.event) {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    window_focus_gained = true;
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    applog_d("SDL", "Window event SDL_WINDOWEVENT_FOCUS_LOST");
#if !FEATURE_FORCE_FULLSCREEN
                    if (app_configuration->fullscreen) {
                        // Interrupt streaming because app will go to background
                        streaming_interrupt(false, STREAMING_INTERRUPT_BACKGROUND);
                    }
#endif
                    window_focus_gained = false;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    lv_app_display_resize(lv_disp_get_default(), event->window.data1, event->window.data2);
                    ui_display_size(event->window.data1, event->window.data2);
                    bus_pushevent(USER_SIZE_CHANGED, NULL, NULL);
                    break;
                case SDL_WINDOWEVENT_HIDDEN:
                    applog_d("SDL", "Window event SDL_WINDOWEVENT_HIDDEN");
                    if (app_configuration->fullscreen) {
                        // Interrupt streaming because app will go to background
                        streaming_interrupt(false, STREAMING_INTERRUPT_BACKGROUND);
                    }
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    lv_obj_invalidate(lv_scr_act());
                    break;
                default:
                    break;
            }
            break;
        }
        case SDL_USEREVENT: {
            if (event->user.code == BUS_INT_EVENT_ACTION) {
                bus_actionfunc actionfn = event->user.data1;
                actionfn(event->user.data2);
            } else {
                bool handled = backend_dispatch_userevent(event->user.code, event->user.data1, event->user.data2);
                handled = handled || ui_dispatch_userevent(event->user.code, event->user.data1, event->user.data2);
                if (!handled) {
                    applog_w("Event", "Nobody handles event %d", event->user.code);
                }
            }
            break;
        }
        case SDL_QUIT: {
            app_request_exit();
            break;
        }
        case SDL_JOYDEVICEADDED:
        case SDL_JOYDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEREMAPPED: {
            inputmgr_sdl_handle_event(event);
            break;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERAXISMOTION:
        case SDL_TEXTINPUT:
            return 1;
        default:
            if (event->type == USER_REMOTEBUTTONEVENT) {
                return 1;
            }
            return 0;
    }
    return 0;
}

void app_process_events() {
    SDL_PumpEvents();
    SDL_FilterEvents(app_event_filter, NULL);
}


void app_stop_text_input() {
    SDL_StopTextInput();
}

void app_set_mouse_grab(bool grab) {
#if HAVE_RELATIVE_MOUSE_HACK
    if (grab) {
        applog_d("Input", "Set cursor to blank bitmap: %p", blank_cursor);
        SDL_SetCursor(blank_cursor);
    } else {
        SDL_SetCursor(SDL_GetDefaultCursor());
    }
#else
    SDL_SetRelativeMouseMode(grab && !app_configuration->absmouse ? SDL_TRUE : SDL_FALSE);
    if (!grab) {
        SDL_ShowCursor(SDL_TRUE);
    }
#endif
}

bool app_get_mouse_relative() {
#if HAVE_RELATIVE_MOUSE_HACK
    return !app_configuration->absmouse;
#else
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
#endif
}

void app_set_keep_awake(bool awake) {
    if (awake) {
        SDL_DisableScreenSaver();
    } else {
        SDL_EnableScreenSaver();
    }
}

void app_quit_confirm() {
    static const char *btn_txts[] = {translatable("Cancel"), translatable("OK"), ""};
    lv_obj_t *mbox = lv_msgbox_create_i18n(NULL, NULL, locstr("Quit Moonlight?"), btn_txts, false);
    lv_obj_add_event_cb(mbox, quit_confirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(mbox);
}

static void quit_confirm_cb(lv_event_t *e) {
    lv_obj_t *mbox = lv_event_get_current_target(e);
    if (lv_msgbox_get_active_btn(mbox) == 1) {
        app_request_exit();
    } else {
        lv_msgbox_close_async(mbox);
    }
}