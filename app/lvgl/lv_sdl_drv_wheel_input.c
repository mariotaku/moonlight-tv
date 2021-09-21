#include <SDL.h>
#include <stream/input/sdlinput.h>
#include "lv_sdl_drv_wheel_input.h"

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

lv_indev_t *lv_sdl_init_wheel() {
    lv_group_t *group = lv_group_get_default();

    lv_indev_drv_t *indev_drv = malloc(sizeof(lv_indev_drv_t));
    lv_indev_drv_init(indev_drv);
    indev_drv->type = LV_INDEV_TYPE_ENCODER;
    indev_drv->read_cb = sdl_input_read;

    lv_indev_t *indev = lv_indev_drv_register(indev_drv);
    if (group) {
        lv_indev_set_group(indev, group);
    }
    return indev;
}

void lv_sdl_deinit_wheel(lv_indev_t *dev) {
    free(dev->driver);
}

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    SDL_Event e;
    data->state = LV_INDEV_STATE_RELEASED;
    if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEWHEEL, SDL_MOUSEWHEEL)) {
        absinput_dispatch_event(&e);
#if TARGET_WEBOS
        data->enc_diff = (short) e.wheel.y;
#else
        data->enc_diff = 0;
#endif
        data->continue_reading = true;
    } else {
        data->enc_diff = 0;
        data->continue_reading = false;
    }
}