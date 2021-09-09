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
#include "lvgl/lv_sdl_img.h"

#include "debughelper.h"
#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "lvgl/lv_obj_controller.h"
#include "ui/root.h"
#include "util/bus.h"
#include "util/logging.h"

#include <fontconfig/fontconfig.h>
#include <ui/launcher/launcher.controller.h>

FILE *app_logfile = NULL;

static bool running = true;
static SDL_mutex *app_gs_client_mutex = NULL;

static void lv_bg_draw(lv_area_t *area);

lv_controller_manager_t *app_uimanager;

lv_indev_t *app_indev_key;

int main(int argc, char *argv[]) {
    app_loginit();
#if TARGET_WEBOS || TARGET_LGNC
    app_logfile = fopen("/tmp/" APPID ".log", "a+");
    if (!app_logfile)
        app_logfile = stdout;
    setvbuf(app_logfile, NULL, _IONBF, 0);
    if (getenv("MOONLIGHT_OUTPUT_NOREDIR") == NULL)
        REDIR_STDOUT(APPID);
#else
    setvbuf(stdout, NULL, _IONBF, 0);
#endif
    applog_d("APP", "Start Moonlight. Version %s", APP_VERSION);
    app_gs_client_mutex = SDL_CreateMutex();

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
    Uint32 window_flags = 0;
#ifdef TARGET_WEBOS
    window_flags |= SDL_WINDOW_FULLSCREEN;
#else
    if (app_configuration->fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
#endif
    SDL_Window *window = SDL_CreateWindow("Moonlight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1920, 1080, window_flags);
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

    lv_img_decoder_t *img_decoder = lv_img_decoder_create();
    lv_sdl_img_decoder_init(img_decoder);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    lv_indev_t *indev_key = lv_sdl_init_key_input();
    lv_indev_t *indev_pointer = lv_sdl_init_pointer();
    app_indev_key = indev_key;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_opa(scr, 0, 0);
    app_uimanager = lv_controller_manager_create(scr);
    lv_controller_manager_push(app_uimanager, &launcher_controller_class, NULL);

    while (running) {
        app_process_events();
        lv_task_handler();
        SDL_Delay(1);
    }

    settings_save(app_configuration);

    lv_controller_manager_del(app_uimanager);

    lv_sdl_deinit_pointer(indev_pointer);
    lv_sdl_deinit_key_input(indev_key);
    lv_sdl_display_deinit(disp);
//    lv_deinit();

    SDL_DestroyWindow(window);

    backend_destroy();
    app_destroy();

    SDL_DestroyMutex(app_gs_client_mutex);

    applog_d("APP", "Quitted gracefully :)");
    return 0;
}

void app_request_exit() {
    running = false;
}

GS_CLIENT app_gs_client_new() {
    SDL_LockMutex(app_gs_client_mutex);
    SDL_assert(app_configuration);
    GS_CLIENT client = gs_new(app_configuration->key_dir, app_configuration->debug_level);
    SDL_assert(client);
    SDL_LockMutex(app_gs_client_mutex);
    return client;
}

static void lv_bg_draw(lv_area_t *area) {
    SDL_Rect rect = {.x=area->x1, .y= area->y1, .w = lv_area_get_width(area), .h = lv_area_get_height(area)};
    lv_disp_t *disp = lv_disp_get_default();
    lv_disp_drv_t *driver = disp->driver;
    SDL_Renderer *renderer = driver->user_data;
    SDL_assert(SDL_GetRenderTarget(renderer) == lv_disp_get_draw_buf(disp)->buf_act);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_RenderFillRect(renderer, &rect);
}