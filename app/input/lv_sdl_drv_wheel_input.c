#include "lvgl/lv_sdl_drv_input.h"

#include <SDL.h>
#include "stream/input/sdlinput.h"

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

int lv_sdl_init_wheel(lv_indev_drv_t *drv, app_ui_input_t *input) {
    lv_indev_drv_init(drv);
    drv->user_data = input;
    drv->type = LV_INDEV_TYPE_ENCODER;
    drv->read_cb = sdl_input_read;

    return 0;
}

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    SDL_Event e;
    data->state = LV_INDEV_STATE_RELEASED;
    if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEWHEEL, SDL_MOUSEWHEEL)) {
        absinput_dispatch_event(&e);
    }
    data->enc_diff = 0;
    data->continue_reading = false;
}