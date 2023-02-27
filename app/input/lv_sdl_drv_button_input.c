#include "lvgl/lv_sdl_drv_input.h"

#include <SDL.h>
#include "stream/input/sdlinput.h"
#include "util/user_event.h"

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

unsigned int app_userevent_remotebutton = 0;

int lv_sdl_init_button(lv_indev_drv_t *indev_drv) {
    app_userevent_remotebutton = SDL_RegisterEvents(1);

    lv_indev_drv_init(indev_drv);
    indev_drv->type = LV_INDEV_TYPE_BUTTON;
    indev_drv->read_cb = sdl_input_read;
    return 0;
}

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    SDL_Event e;
    data->state = LV_INDEV_STATE_RELEASED;
    if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, USER_REMOTEBUTTONEVENT, USER_REMOTEBUTTONEVENT)) {
        switch (e.key.keysym.scancode) {
#if TARGET_WEBOS
            case SDL_WEBOS_SCANCODE_RED:
                data->btn_id = 1;
                data->continue_reading = true;
                break;
            case SDL_WEBOS_SCANCODE_GREEN:
                data->btn_id = 2;
                data->continue_reading = true;
                break;
            case SDL_WEBOS_SCANCODE_YELLOW:
                data->btn_id = 3;
                data->continue_reading = true;
                break;
            case SDL_WEBOS_SCANCODE_BLUE:
                data->btn_id = 4;
                data->continue_reading = true;
                break;
#endif
            default:
                break;
        }
        if (data->btn_id) {
            data->state = e.key.state == SDL_PRESSED;
        }
    } else {
        data->continue_reading = false;
    }
}