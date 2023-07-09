#include "lvgl/lv_sdl_drv_input.h"

#include "app.h"

#include <SDL.h>
#include "stream/input/sdlinput.h"
#include "ui/root.h"
#include "logging.h"

#include "lvgl.h"
#include "stream/session.h"
#include "stream/session/session_events.h"

static void indev_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

static void indev_point_def(lv_indev_data_t *data);

int lv_sdl_init_pointer(lv_indev_drv_t *drv, app_ui_input_t *input) {
    lv_indev_drv_init(drv);
    drv->user_data = input;
    drv->type = LV_INDEV_TYPE_POINTER;
    drv->read_cb = indev_pointer_read;
    return 0;
}

static void indev_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    app_ui_input_t *input = drv->user_data;
    app_t *app = input->ui->app;
    SDL_Event e;
    data->continue_reading = SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP) > 0;
    if (!data->continue_reading) {
        indev_point_def(data);
        return;
    }
    static bool warped = false;
    if (e.type == SDL_MOUSEMOTION) {
        if (!warped) {
            session_handle_input_event(app->session, &e);
        }
        data->state = e.motion.state;
        data->point = (lv_point_t) {.x = e.motion.x, .y = e.motion.y};
#if HAVE_RELATIVE_MOUSE_HACK
        if (warped) {
            warped = false;
        } else if (session_accepting_input(app->session) && app_get_mouse_relative()) {
            int w, h;
            SDL_GetWindowSize(input->ui->window, &w, &h);
            SDL_WarpMouseInWindow(input->ui->window, w / 2, h / 2);
            warped = true;
        }
#endif
    } else {
        session_handle_input_event(app->session, &e);
        data->state = e.button.state;
        data->point = (lv_point_t) {.x = e.button.x, .y = e.button.y};
        ui_set_input_mode(input, UI_INPUT_MODE_MOUSE);
    }
}

static void indev_point_def(lv_indev_data_t *data) {
    int x, y;
    data->state = SDL_GetMouseState(&x, &y) != 0;
    data->point.x = (lv_coord_t) x;
    data->point.y = (lv_coord_t) y;
}