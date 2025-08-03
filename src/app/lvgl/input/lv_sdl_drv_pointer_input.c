#include "lvgl/lv_sdl_drv_input.h"

#include "app.h"

#include <SDL.h>
#include "ui/root.h"
#include "logging.h"

#include "lvgl.h"
#include "stream/session.h"
#include "stream/session_events.h"

static void indev_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

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
    static lv_point_t point = {0, 0};
    static lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
    if (!data->continue_reading) {
        data->point = point;
        data->state = state;
        return;
    }
    if (e.type == SDL_MOUSEMOTION) {
        if (app->session != NULL) {
            session_handle_input_event(app->session, &e);
        }
        state = data->state = e.motion.state;
        point = data->point = (lv_point_t) {.x = e.motion.x, .y = e.motion.y};
    } else {
        if (app->session != NULL) {
            session_handle_input_event(app->session, &e);
        }
        state = data->state = e.button.state;
        point = data->point = (lv_point_t) {.x = e.button.x, .y = e.button.y};
        ui_set_input_mode(input, UI_INPUT_MODE_MOUSE);
    }
}