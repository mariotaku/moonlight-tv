#include "app.h"
#include "root.h"
#include "res.h"

#include "lvgl/lv_disp_drv_app.h"
#include "lvgl/theme/lv_theme_moonlight.h"
#include "draw/sdl/lv_draw_sdl_utils.h"

#include "stream/platform.h"
#include "stream/session.h"
#include "stream/input/absinput.h"

#include "streaming/streaming.controller.h"
#include "launcher/launcher.controller.h"

#include "logging.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "util/font.h"

#include <SDL_image.h>

#include "logging_ext_lvgl.h"
#include "fatal_error.h"

#if FEATURE_WINDOW_FULLSCREEN_DESKTOP
#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#else
#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN
#endif

static SDL_AssertState app_assertion_handler_ui(const SDL_AssertData *data, void *userdata);

short ui_display_width, ui_display_height;
static unsigned int last_pts = 0;

enum UI_INPUT_MODE ui_input_mode;
lv_img_dsc_t ui_logo_src_ = {.data_size = 0};

typedef struct render_frame_req_t {
    void *data;
    unsigned int pts;
} render_frame_req_t;

SDL_Window *app_create_window();

void app_ui_init(app_ui_t *ui, app_t *app) {
    ui->window = app_create_window();
    lv_log_register_print_cb(commons_lv_log);
    lv_init();
    ui->img_decoder = lv_sdl_img_decoder_init(IMG_INIT_JPG | IMG_INIT_PNG);
    lv_memset_00(&ui->theme, sizeof(lv_theme_t));
    lv_theme_moonlight_init(&ui->theme, app);
    ui->fonts = app_font_init(&ui->theme);
}

void app_ui_deinit(app_ui_t *ui) {
    if (!app_configuration->fullscreen) {
        SDL_GetWindowPosition(ui->window, &app_configuration->window_state.x, &app_configuration->window_state.y);
        SDL_GetWindowSize(ui->window, &app_configuration->window_state.w, &app_configuration->window_state.h);
    }
    app_font_deinit(ui->fonts);

    SDL_DestroyWindow(ui->window);
    lv_img_decoder_delete(ui->img_decoder);
}

void app_ui_open(app_ui_t *ui) {
    ui->disp = lv_app_display_init(ui->window);

    lv_theme_t *parent_theme = lv_disp_get_theme(ui->disp);
    ui->theme.color_primary = parent_theme->color_primary;
    ui->theme.color_secondary = parent_theme->color_secondary;
    lv_theme_set_parent(&ui->theme, parent_theme);
    lv_disp_set_theme(ui->disp, &ui->theme);
    streaming_display_size(ui->disp->driver->hor_res, ui->disp->driver->ver_res);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    app_ui_input_init(&ui->input, ui);

    SDL_SetAssertionHandler(app_assertion_handler_ui, ui->app);
}

void app_ui_close(app_ui_t *ui) {
    SDL_SetAssertionHandler(app_assertion_handler_abort, NULL);

    lv_fragment_manager_del(app_uimanager);

    app_ui_input_deinit(&ui->input);
    lv_app_display_deinit(ui->disp);
    ui->disp = NULL;
}

bool ui_has_stream_renderer() {
//    return ui_stream_render != NULL && ui_stream_render->renderDraw;
    return false;
}

bool ui_render_background() {
//    if (streaming_status == STREAMING_STREAMING && ui_stream_render && ui_stream_render->renderDraw) {
//        return ui_stream_render->renderDraw();
//    }
    return false;
}

bool ui_dispatch_userevent(app_t *app, int which, void *data1, void *data2) {
    bool handled = false;
    ui_userevent_t userdata = {data1, data2};
    handled |= lv_fragment_manager_send_event(app_uimanager, which, &userdata);
    if (!handled) {
        switch (which) {
            case USER_STREAM_OPEN: {
                lv_draw_sdl_drv_param_t *param = lv_disp_get_default()->driver->user_data;
                // TODO: setup renderer
//                ui_stream_render_host_context.renderer = param->renderer;
//                ui_stream_render = decoder_get_render();
//                if (ui_stream_render) {
//                    ui_stream_render->renderSetup((PSTREAM_CONFIGURATION) data1, &ui_stream_render_host_context);
//                }
                absinput_start();
                app_set_keep_awake(true);
                streaming_enter_fullscreen();
                last_pts = 0;
                return true;
            }
            case USER_STREAM_CLOSE:
                // TODO: close renderer
//                if (ui_stream_render) {
//                    ui_stream_render->renderCleanup();
//                }
                app_set_keep_awake(false);
                app_set_mouse_grab(false);
                absinput_stop();
//                ui_stream_render = NULL;
//                ui_stream_render_host_context.renderer = NULL;
                return true;
            default:
                break;
        }
    }
    return handled;
}

bool ui_should_block_input() {
    return streaming_overlay_shown();
}

void ui_display_size(short width, short height) {
    ui_display_width = width;
    ui_display_height = height;
    commons_log_info("UI", "Display size changed to %d x %d", width, height);
}

bool ui_set_input_mode(enum UI_INPUT_MODE mode) {
    if (ui_input_mode == mode) {
        return false;
    }
    ui_input_mode = mode;
    return true;
}

const lv_img_dsc_t *ui_logo_src() {
    if (ui_logo_src_.data_size == 0) {
        ui_logo_src_.header.w = LV_DPX(NAV_LOGO_SIZE);
        ui_logo_src_.header.h = LV_DPX(NAV_LOGO_SIZE);
        ui_logo_src_.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;

        ui_logo_src_.data_size = sizeof(lv_sdl_img_data_logo_96);
        ui_logo_src_.data = (const uint8_t *) &lv_sdl_img_data_logo_96;
    }
    return &ui_logo_src_;
}

static void handle_queued_frame(render_frame_req_t *req) {
    bool redraw = false;
    if (last_pts > req->pts) {
        commons_log_warn("Stream", "Ignore late frame pts: %d", req->pts);
        return;
    }
//    if (ui_stream_render && ui_stream_render->renderSubmit(req->data)) {
//        redraw = true;
//    }
    if (redraw) {
        lv_app_redraw_now(lv_disp_get_default()->driver);
    }
}

bool ui_render_queue_submit(void *data, unsigned int pts) {
//    commons_log_debug("Stream", "Submit frame. pts: %d", pts);
    render_frame_req_t req = {.data = data, .pts = pts};
    bus_pushaction_sync((bus_actionfunc) handle_queued_frame, &req);
    return true;
}

void ui_cb_destroy_fragment(lv_event_t *e) {
    lv_fragment_t *fragment = lv_event_get_user_data(e);
    if (fragment->cls->obj_deleted_cb) {
        fragment->cls->obj_deleted_cb(fragment, lv_event_get_target(e));
    }
    fragment->obj = NULL;
    lv_fragment_del(fragment);
}

SDL_Window *app_create_window() {
    Uint32 win_flags = SDL_WINDOW_RESIZABLE;
    int win_x = SDL_WINDOWPOS_UNDEFINED, win_y = SDL_WINDOWPOS_UNDEFINED,
            win_width = 1920, win_height = 1080;
    if (app_configuration->fullscreen) {
        win_flags |= APP_FULLSCREEN_FLAG;
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(0, 0, &mode);
        if (mode.w > 0 && mode.h > 0) {
            win_width = mode.w;
            win_height = mode.h;
        }
    } else {
        win_x = app_configuration->window_state.x;
        win_y = app_configuration->window_state.y;
        win_width = app_configuration->window_state.w;
        win_height = app_configuration->window_state.h;
    }
    SDL_Window *win = SDL_CreateWindow("Moonlight", win_x, win_y, win_width, win_height, win_flags);
    SDL_assert_release(win != NULL);

    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(lv_sdl_img_data_logo_96.data.constptr,
                                                          (int) lv_sdl_img_data_logo_96.data_len), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
    SDL_SetWindowMinimumSize(win, 640, 480);
    int w = 0, h = 0;
    SDL_GetWindowSize(win, &w, &h);
    SDL_assert_release(w > 0 && h > 0);
    ui_display_size(w, h);
    return win;
}

void app_set_fullscreen(app_t *app, bool fullscreen) {
    SDL_SetWindowFullscreen(app->ui.window, fullscreen ? APP_FULLSCREEN_FLAG : 0);
}

static SDL_AssertState app_assertion_handler_ui(const SDL_AssertData *data, void *userdata) {
    (void) userdata;
    app_fatal_error("Assertion failure", "at %s\n(%s:%d): '%s'", data->function, data->filename, data->linenum,
                    data->condition);
    return SDL_ASSERTION_ALWAYS_IGNORE;
}
