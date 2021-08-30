#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "app.h"

#define RES_IMPL

#include "res.h"

#undef RES_IMPL

#include "ui/config.h"

#include "lvgl.h"
#include "gpu/lv_gpu_sdl.h"
#include "lvgl/lv_sdl_drv_key_input.h"
#include "lvgl/lv_sdl_drv_pointer_input.h"

#include "debughelper.h"
#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/manager.h"
#include "ui/root.h"
#include "util/bus.h"
#include "util/logging.h"

#include <fontconfig/fontconfig.h>
#include <ui/launcher/window.h>

FILE *app_logfile = NULL;

static bool running = true;
static GS_CLIENT app_gs_client = NULL;
static pthread_mutex_t app_gs_client_mutex = PTHREAD_MUTEX_INITIALIZER;

static void app_gs_client_destroy();

static void lv_bg_draw(lv_area_t *area);

uimanager_ctx *app_uimanager;

int main(int argc, char *argv[]) {
    app_loginit();
#if TARGET_WEBOS || TARGET_LGNC
    app_logfile = fopen("/tmp/" APPID ".log", "a+");
    if (!app_logfile)
        app_logfile = stdout;
    setvbuf(app_logfile, NULL, _IONBF, 0);
    if (getenv("MOONLIGHT_OUTPUT_NOREDIR") == NULL)
        REDIR_STDOUT(APPID);
    applog_d("APP", "Start Moonlight. Version %s", APP_VERSION);
#endif
    bus_init();

    int ret = app_init(argc, argv);
    if (ret != 0) {
        return ret;
    }
    module_host_context.logvprintf = &app_logvprintf;

    // LGNC requires init before window created, don't put this after app_window_create!
    decoder_init(app_configuration->decoder, argc, argv);
    audio_init(app_configuration->audio_backend, argc, argv);

    applog_i("APP", "Decoder module: %s (%s requested)", decoder_definitions[decoder_current].name,
             app_configuration->decoder);
    if (audio_current == AUDIO_DECODER) {
        applog_i("APP", "Audio module: decoder implementation (%s requested)\n", app_configuration->audio_backend);
    } else if (audio_current >= 0) {
        applog_i("APP", "Audio module: %s (%s requested)", audio_definitions[audio_current].name,
                 app_configuration->audio_backend);
    }

    backend_init();

    SDL_Window *window = SDL_CreateWindow("Moonlight", 0, 0, 1920, 1080, SDL_WINDOW_FULLSCREEN_DESKTOP);
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    ui_display_size(w, h);

    lv_init();
    lv_disp_t *disp = lv_sdl_display_init(window);
    disp->bg_color = lv_color_make(0, 0, 0);
    disp->bg_opa = 0;
    disp->bg_fn = lv_bg_draw;
    disp->theme->font_small = &lv_font_montserrat_24;
    disp->theme->font_large = &lv_font_montserrat_32;
    streaming_display_size(disp->driver->hor_res, disp->driver->ver_res);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    lv_sdl_init_key_input();
    lv_indev_t *indev_pointer = lv_sdl_init_pointer();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_opa(scr, 0, 0);
    app_uimanager = uimanager_new(scr);
    uimanager_push(app_uimanager, launcher_controller, NULL);

    while (running) {
        app_process_events();
        lv_task_handler();
        SDL_Delay(1);
    }

    settings_save(app_configuration);

    uimanager_destroy(app_uimanager);

    lv_sdl_deinit_pointer(indev_pointer);
    lv_sdl_deinit_key_input(NULL);
    lv_sdl_display_deinit(disp);
//    lv_deinit();

    SDL_DestroyWindow(window);

    backend_destroy();
    app_gs_client_destroy();
    app_destroy();
    bus_destroy();

    applog_d("APP", "Quitted gracefully :)");
}

void app_request_exit() {
    running = false;
}

GS_CLIENT app_gs_client_obtain() {
    pthread_mutex_lock(&app_gs_client_mutex);
    assert(app_configuration);
    if (!app_gs_client)
        app_gs_client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    assert(app_gs_client);
    pthread_mutex_unlock(&app_gs_client_mutex);
    return app_gs_client;
}

void app_gs_client_destroy() {
    if (app_gs_client) {
        gs_destroy(app_gs_client);
        app_gs_client = NULL;
    }
    // Further calls to obtain gs client will be locked
    pthread_mutex_lock(&app_gs_client_mutex);
}

bool app_gs_client_ready() {
    return app_gs_client != NULL;
}

static void lv_bg_draw(lv_area_t *area) {
    SDL_Rect rect = {.x=area->x1, .y= area->x1, .w = lv_area_get_width(area), .h = lv_area_get_height(area)};
    lv_disp_t *disp = lv_disp_get_default();
    lv_disp_drv_t *driver = disp->driver;
    if (rect.w != driver->hor_res || rect.h != driver->ver_res) return;
    SDL_Renderer *renderer = driver->user_data;
    SDL_assert(SDL_GetRenderTarget(renderer) == lv_disp_get_draw_buf(disp)->buf_act);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_RenderFillRect(renderer, &rect);
}