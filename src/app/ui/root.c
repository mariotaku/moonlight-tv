#include "app.h"
#include "root.h"
#include "res.h"

#include "lvgl/lv_disp_drv_app.h"
#include "lvgl/theme/lv_theme_moonlight.h"
#include "draw/sdl/lv_draw_sdl_utils.h"

#include "stream/session.h"
#include "stream/input/session_input.h"

#include "streaming/streaming.controller.h"
#include "launcher/launcher.controller.h"

#include "logging.h"
#include "util/bus.h"
#include "util/user_event.h"
#include "util/font.h"

#include <SDL_image.h>

#include "logging_ext_lvgl.h"
#include "fatal_error.h"
#include "app_error.h"
#include "util/i18n.h"
#include "lvgl/util/lv_app_utils.h"

#if FEATURE_WINDOW_FULLSCREEN_DESKTOP
#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#else
#define APP_FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN
#endif

static SDL_AssertState app_assertion_handler_ui(const SDL_AssertData *data, void *userdata);

static unsigned int last_pts = 0;

lv_img_dsc_t ui_logo_src_ = {.data_size = 0};

typedef struct render_frame_req_t {
    void *data;
    unsigned int pts;
} render_frame_req_t;

SDL_Window *app_ui_create_window(app_ui_t *ui);

static void session_error();

static void session_error_dialog_cb(lv_event_t *event);

void app_ui_init(app_ui_t *ui, app_t *app) {
    ui->app = app;
    ui->window = app_ui_create_window(ui);
    lv_log_register_print_cb(commons_lv_log);
    lv_init();
    ui->img_decoder = lv_sdl_img_decoder_init(IMG_INIT_JPG | IMG_INIT_PNG);
    app_font_init(&ui->fonts, ui->dpi);
    lv_memset_00(&ui->theme, sizeof(lv_theme_t));
    lv_theme_moonlight_init(&ui->theme, &ui->fonts, app);
}

void app_ui_deinit(app_ui_t *ui) {
    if (!app_configuration->fullscreen) {
        SDL_GetWindowPosition(ui->window, &app_configuration->window_state.x, &app_configuration->window_state.y);
        SDL_GetWindowSize(ui->window, &app_configuration->window_state.w, &app_configuration->window_state.h);
    }
    lv_theme_moonlight_deinit(&ui->theme);
    app_font_deinit(&ui->fonts);
    lv_img_decoder_delete(ui->img_decoder);
}

void app_ui_open(app_ui_t *ui, bool open_launcher, const app_launch_params_t *params) {
    if (ui->disp != NULL) {
        return;
    }
    if (ui->window == NULL) {
        ui->window = app_ui_create_window(ui);
    }
    lv_disp_drv_t *driver = lv_app_disp_drv_create(ui->window, ui->dpi);
    lv_disp_t *disp = lv_disp_drv_register(driver);
    disp->bg_color = lv_color_make(0, 0, 0);
    disp->bg_opa = 0;
    ui->disp = disp;

    lv_theme_t *parent_theme = lv_disp_get_theme(ui->disp);
    ui->theme.color_primary = parent_theme->color_primary;
    ui->theme.color_secondary = parent_theme->color_secondary;
    lv_theme_set_parent(&ui->theme, parent_theme);
    lv_disp_set_theme(ui->disp, &ui->theme);

    lv_group_t *group = lv_group_create();
    lv_group_set_editing(group, 0);
    lv_group_set_default(group);
    app_ui_input_init(&ui->input, ui);

    ui->container = lv_scr_act();
    lv_obj_clear_flag(ui->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui->container, 0, 0);
    ui->fm = lv_fragment_manager_create(NULL);
    if (open_launcher) {
        launcher_fragment_args_t args = {.app = ui->app, .params = params};
        lv_fragment_t *fragment = lv_fragment_create(&launcher_controller_class, &args);
        lv_fragment_manager_push(ui->fm, fragment, &ui->container);
    }

    SDL_SetAssertionHandler(app_assertion_handler_ui, ui->app);

    app_set_keep_awake(ui->app, false);
}

void app_ui_close(app_ui_t *ui) {
    SDL_SetAssertionHandler(app_assertion_handler_abort, NULL);
    if (ui->disp == NULL) {
        return;
    }

    lv_fragment_manager_del(ui->fm);

    ui->fm = NULL;
    ui->container = NULL;

    app_ui_input_deinit(&ui->input);
    lv_theme_set_parent(&ui->theme, NULL);
    lv_disp_drv_t *driver = ui->disp->driver;
    lv_disp_set_default(NULL);
    lv_disp_remove(ui->disp);
    lv_app_disp_drv_deinit(driver);
    ui->disp = NULL;
    SDL_DestroyWindow(ui->window);
    ui->window = NULL;
}

bool app_ui_is_opened(const app_ui_t *ui) {
    return ui->disp != NULL;
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
    handled |= app->ui.fm != NULL && lv_fragment_manager_send_event(app->ui.fm, which, &userdata);
    if (!handled) {
        switch (which) {
            case USER_STREAM_OPEN: {
                if (app->ss4s.video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_UI_EXCLUSIVE) {
                    SDL_ShowCursor(SDL_FALSE);
                } else {
                    lv_draw_sdl_drv_param_t *param = lv_disp_get_default()->driver->user_data;
                    // TODO: setup renderer
//                ui_stream_render_host_context.renderer = param->renderer;
//                ui_stream_render = decoder_get_render();
//                if (ui_stream_render) {
//                    ui_stream_render->renderSetup((PSTREAM_CONFIGURATION) data1, &ui_stream_render_host_context);
//                }
                }
                if (session_start_input(app->session)) {
                    app_set_mouse_grab(&app->input, true);
                }
                app_set_keep_awake(app, true);
                streaming_enter_fullscreen(app->session);
                last_pts = 0;
                return true;
            }
            case USER_STREAM_CLOSE: {
                if (app->ss4s.video_cap.transform & SS4S_VIDEO_CAP_TRANSFORM_UI_EXCLUSIVE) {
                    SDL_ShowCursor(SDL_TRUE);
                } else {

                }
                // TODO: close renderer
//                if (ui_stream_render) {
//                    ui_stream_render->renderCleanup();
//                }
                app_set_keep_awake(app, false);
                if (session_has_input(app->session)) {
                    app_set_mouse_grab(&app->input, false);
                    session_stop_input(app->session);
                }
//                ui_stream_render = NULL;
//                ui_stream_render_host_context.renderer = NULL;
                app_ui_open(&app->ui, true, NULL);
                return true;
            }
            case USER_STREAM_FINISHED: {
                if (streaming_errno != 0) {
                    session_error();
                    break;
                }
                return true;
            }
            case USER_OPEN_OVERLAY: {
                if (app->session != NULL && !app_ui_is_opened(&app->ui)) {
                    session_interrupt(app->session, false, STREAMING_INTERRUPT_USER);
                    return true;
                }
                return false;
            }
            default:
                break;
        }
    }
    return handled;
}

bool ui_should_block_input() {
    return streaming_overlay_shown();
}

void ui_display_size(app_ui_t *ui, int width, int height) {
    ui->width = width;
    ui->height = height;
    if (ui->dpi <= 0) {
        ui->dpi = width / 6;
    }
    commons_log_info("UI", "Display size changed to %d x %d", width, height);
}

bool ui_set_input_mode(app_ui_input_t *input, app_ui_input_mode_t mode) {
    if (input->mode == mode) {
        return false;
    }
    input->mode = mode;
    return true;
}

app_ui_input_mode_t app_ui_get_input_mode(const app_ui_input_t *input) {
    return input->mode;
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
    app_bus_post_sync(global, (bus_actionfunc) handle_queued_frame, &req);
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

SDL_Window *app_ui_create_window(app_ui_t *ui) {
    Uint32 win_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
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
    if (win == NULL) {
#ifdef TARGET_WEBOS
        // For webOS 24 (9.0), unpopulated jailer config could cause graphics driver initialization failure
        // We don't have any way to work around this, so we try to launch a web page about this issue
        if (ui->app->os_info.version.major >= 9) {
            SDL_OpenURL("https://github.com/mariotaku/moonlight-tv/wiki/Known-Issues");
        }
#endif
        commons_log_fatal("APP", "Failed to create window: %s", SDL_GetError());
        app_halt(ui->app);
    }

    SDL_Surface *winicon = IMG_Load_RW(SDL_RWFromConstMem(lv_sdl_img_data_logo_96.data.constptr,
                                                          (int) lv_sdl_img_data_logo_96.data_len), SDL_TRUE);
    SDL_SetWindowIcon(win, winicon);
    SDL_FreeSurface(winicon);
    if (app_configuration->fullscreen) {
        SDL_SetWindowMinimumSize(win, win_width, win_height);
    } else {
        SDL_SetWindowMinimumSize(win, 640, 480);
    }
    int w = 0, h = 0;
    SDL_GetWindowSize(win, &w, &h);
    SDL_assert_release(w > 0 && h > 0);
    ui_display_size(ui, w, h);
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

static void session_error() {
    static const char *btn_texts[] = {translatable("OK"), ""};
    lv_obj_t *dialog = lv_msgbox_create_i18n(NULL, locstr("Failed to start streaming"), streaming_errmsg,
                                             btn_texts, false);
    lv_obj_add_event_cb(dialog, session_error_dialog_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(dialog);
}

static void session_error_dialog_cb(lv_event_t *event) {
    lv_obj_t *dialog = lv_event_get_current_target(event);
    lv_msgbox_close_async(dialog);
}
