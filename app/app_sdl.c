#include <stdbool.h>
#include <SDL.h>

#include "app.h"
#include "res.h"

#include "ui/config.h"

#if TARGET_DESKTOP || TARGET_RASPI

#include <SDL_image.h>
#include <gpu/lv_gpu_sdl.h>
#include <lvgl/lv_disp_drv_app.h>

#endif

#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/input/absinput.h"
#include "stream/input/sdlinput.h"
#include "stream/platform.h"
#include "platform/sdl/events.h"
#include "platform/sdl/navkey_sdl.h"
#include "ui/root.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "util/logging.h"

#if TARGET_WEBOS
#define FORCE_FULLSCREEN
#elif TARGET_RASPI
#define FORCE_FULLSCREEN
#endif

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

/* Platform */
SDL_Window *win;
static SDL_GLContext gl;
int app_window_width, app_window_height;
bool app_should_redraw_background = false;

PCONFIGURATION app_configuration = NULL;

static char wintitle[32];

static bool window_focus_gained;

static void fps_cap(int diff);

static void applog_logoutput(void *, int category, SDL_LogPriority priority, const char *message);

int app_init(int argc, char *argv[]) {
    app_configuration = settings_load();
#if TARGET_WEBOS
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
    SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT, "true");
    SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
#endif
    return 0;
}

APP_WINDOW_CONTEXT app_window_create() {
    SDL_LogSetOutputFunction(applog_logoutput, NULL);
#if DEBUG
    // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#endif
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef FORCE_FULLSCREEN
    SDL_DisplayMode dm;
    applog_d("SDL", "SDL_GetCurrentDisplayMode");
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        applog_w("SDL", "SDL_GetCurrentDisplayMode failed. %s", SDL_GetError());
        // Fix low net_fps for rpi4
        dm.w = 1920;
        dm.h = 1080;
    }
    if (!dm.w || !dm.h) {
        dm.w = 1280;
        dm.h = 720;
    }
    applog_d("SDL", "SDL_DisplayMode(w=%d, h=%d)", dm.w, dm.h);
    app_window_width = dm.w;
    app_window_height = dm.h;
#if TARGET_WEBOS || TARGET_LGNC
    window_flags |= SDL_WINDOW_FULLSCREEN;
#else
    window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
    applog_d("SDL", "Selected video driver: %s", SDL_GetCurrentVideoDriver());
#else
    app_window_width = WINDOW_WIDTH;
    app_window_height = WINDOW_HEIGHT;
    window_flags |= SDL_WINDOW_RESIZABLE;
#endif
    win = SDL_CreateWindow("Moonlight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           app_window_width, app_window_height, window_flags);
    if (!win) {
        applog_f("SDL", "SDL_CreateWindow failed. %s", SDL_GetError());
        return NULL;
    }
    applog_i("SDL", "Window created. ID=%d", SDL_GetWindowID(win));
#if TARGET_DESKTOP || TARGET_RASPI
    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(res_window_icon_32_data, res_window_icon_32_size), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
    SDL_SetWindowMinimumSize(win, 640, 400);
#endif
    applog_i("SDL", "Video Driver: %s", SDL_GetCurrentVideoDriver());

    gl = SDL_GL_CreateContext(win);
    if (!gl) {
        applog_f("SDL", "SDL_GL_CreateContext failed. %s", SDL_GetError());
        return NULL;
    }

    return win;
}

void app_destroy() {
    decoder_finalize(decoder_current);
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
    settings_free(app_configuration);
}

void inputmgr_sdl_handle_event(SDL_Event *ev);

static int app_event_filter(void *userdata, SDL_Event *event) {
    switch (event->type) {
        case SDL_APP_WILLENTERBACKGROUND: {
            // Interrupt streaming because app will go to background
            streaming_interrupt(false);
            break;
        }
#if TARGET_DESKTOP || TARGET_RASPI
        case SDL_WINDOWEVENT: {
            switch (event->window.event) {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    window_focus_gained = true;
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    applog_d("SDL", "Window event SDL_WINDOWEVENT_FOCUS_LOST");
#if TARGET_RASPI
                    // Interrupt streaming because app will go to background
                    streaming_interrupt(false);
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
#if TARGET_RASPI
                    // Interrupt streaming because app will go to background
                    streaming_interrupt(false);
#endif
                    break;
                default:
                    break;
            }
            break;
        }
#endif
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
            return 0;
    }
    return 0;
}

void app_process_events() {
    SDL_PumpEvents();
    SDL_FilterEvents(app_event_filter, NULL);
}

void app_start_text_input(int x, int y, int w, int h) {
    if (w > 0 && h > 0) {
        struct SDL_Rect rect = {x, y, w, h};
        SDL_SetTextInputRect(&rect);
    }
    SDL_StartTextInput();
}

void app_stop_text_input() {
    SDL_StopTextInput();
}

void app_set_mouse_grab(bool grab) {
    SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
    if (!grab) {
        SDL_ShowCursor(SDL_TRUE);
    }
}

void app_set_keep_awake(bool awake) {
    if (awake) {
        SDL_DisableScreenSaver();
    } else {
        SDL_EnableScreenSaver();
    }
}

void fps_cap(int start) {
    int tickdiff = SDL_GetTicks() - start;
    if (tickdiff > 0 && tickdiff < 16) {
        SDL_Delay(16 - tickdiff);
    }
}

void applog_logoutput(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    const char *priority_name[SDL_NUM_LOG_PRIORITIES] = {"VERBOSE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
#ifndef DEBUG
    if (priority <= SDL_LOG_PRIORITY_INFO)
        return;
#endif
    applog(priority_name[priority], "SDL", message);
}
