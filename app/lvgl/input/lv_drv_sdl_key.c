#include "lvgl.h"
#include "app.h"

#include "stream/session.h"
#include "stream/input/sdlinput.h"
#include "platform/sdl/navkey_sdl.h"
#include "ui/root.h"

#include "util/user_event.h"
#include "lv_drv_sdl_key.h"
#include "stream/session/session_events.h"

static bool read_event(const SDL_Event *event, lv_drv_sdl_key_t *state);

static bool read_keyboard(const SDL_KeyboardEvent *event, lv_drv_sdl_key_t *state);

#if TARGET_WEBOS

static bool read_webos_key(const SDL_KeyboardEvent *event, lv_drv_sdl_key_t *state);

static void webos_key_input_mode(const SDL_KeyboardEvent *event);

#endif

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

int lv_sdl_init_key_input(lv_drv_sdl_key_t *drv, app_ui_input_t *input) {
    lv_memset_00(drv, sizeof(*drv));
    lv_indev_drv_init(&drv->base);
    drv->base.user_data = input;
    drv->base.type = LV_INDEV_TYPE_KEYPAD;
    drv->base.read_cb = sdl_input_read;

    drv->state = LV_INDEV_STATE_RELEASED;
    return 0;
}

void lv_sdl_key_input_release_key(lv_indev_t *indev) {
    lv_drv_sdl_key_t *state = (lv_drv_sdl_key_t *) indev->driver;
    state->state = LV_INDEV_STATE_RELEASED;
}

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    app_ui_input_t *input = drv->user_data;
    app_t *app = input->ui->app;
    lv_drv_sdl_key_t *state = (lv_drv_sdl_key_t *) drv;
    SDL_Event e;
    if (state->text_remain > 0) {
        if (state->state == LV_INDEV_STATE_PRESSED) {
            state->state = LV_INDEV_STATE_RELEASED;
            state->text_remain--;
            data->continue_reading = state->text_remain > 0;
        } else {
            state->key = 0;
            memcpy(&state->key, &state->text[state->text_next],
                   _lv_txt_encoded_size(&state->text[state->text_next]));
            _lv_txt_encoded_next(state->text, &state->text_next);
            state->state = LV_INDEV_STATE_PRESSED;
            data->continue_reading = true;
        }
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP) > 0) {
#if TARGET_WEBOS
        webos_key_input_mode(&e.key);
#endif
        if (app->session != NULL && session_handle_input_event(app->session, &e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            if (read_keyboard(&e.key, state)) {
                ui_set_input_mode(input, UI_INPUT_MODE_KEY);
            }
        }
        data->continue_reading = true;
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP) > 0) {
        if (app->session != NULL && session_handle_input_event(app->session, &e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            if (read_event(&e, state)) {
                ui_set_input_mode(input, UI_INPUT_MODE_GAMEPAD);
            }
        }
        data->continue_reading = true;
    } else if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_TEXTINPUT, SDL_TEXTINPUT) > 0) {
        if (app->session != NULL && session_handle_input_event(app->session, &e)) {
            state->state = LV_INDEV_STATE_RELEASED;
        } else {
            uint8_t size = _lv_txt_get_encoded_length(e.text.text);
            if (size > 0) {
                state->text_len = strlen(e.text.text);
                state->text_remain = size;
                state->text_next = 0;
                SDL_memcpy(state->text, e.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
                state->key = 0;
                state->state = LV_INDEV_STATE_RELEASED;
            }
        }
        data->continue_reading = true;
    } else {
        data->continue_reading = false;
    }
    data->key = state->key;
    data->state = state->state;
}

static bool read_event(const SDL_Event *event, lv_drv_sdl_key_t *state) {
    bool pressed = state->state == LV_INDEV_STATE_RELEASED;
    NAVKEY navkey = navkey_from_sdl(event, &pressed);
    switch (navkey) {
        case NAVKEY_UP:
            state->key = LV_KEY_UP;
            break;
        case NAVKEY_DOWN:
            state->key = LV_KEY_DOWN;
            break;
        case NAVKEY_LEFT:
            state->key = LV_KEY_LEFT;
            break;
        case NAVKEY_RIGHT:
            state->key = LV_KEY_RIGHT;
            break;
        case NAVKEY_MENU:
            state->key = LV_KEY_NEXT;
            break;
        case NAVKEY_CONFIRM:
            state->key = LV_KEY_ENTER;
            break;
        case NAVKEY_CANCEL:
            state->key = LV_KEY_ESC;
            break;
        case NAVKEY_NEGATIVE:
            state->key = LV_KEY_DEL;
            break;
        case NAVKEY_ALTERNATIVE:
            state->key = LV_KEY_BACKSPACE;
            break;
        default:
            switch (event->type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP: {

                }
                default: {
                    break;
                }
            }
            return false;
    }

    handled:
    (void) 0;
    state->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    return true;
}

static bool read_keyboard(const SDL_KeyboardEvent *event, lv_drv_sdl_key_t *state) {
    bool pressed = event->type == SDL_KEYDOWN;
    switch (event->keysym.sym) {
        case SDLK_UP:
            state->key = LV_KEY_UP;
            break;
        case SDLK_DOWN:
            state->key = LV_KEY_DOWN;
            break;
        case SDLK_RIGHT:
            state->key = LV_KEY_RIGHT;
            break;
        case SDLK_LEFT:
            state->key = LV_KEY_LEFT;
            break;
        case SDLK_ESCAPE:
            state->key = LV_KEY_ESC;
            break;
        case SDLK_DELETE:
            state->key = LV_KEY_DEL;
            break;
        case SDLK_BACKSPACE:
            state->key = LV_KEY_BACKSPACE;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_RETURN2:
            state->key = LV_KEY_ENTER;
            break;
        case SDLK_TAB:
            state->key = (event->keysym.mod & KMOD_SHIFT) ? LV_KEY_PREV : LV_KEY_NEXT;
            break;
        case SDLK_HOME:
            state->key = LV_KEY_HOME;
            break;
        case SDLK_END:
            state->key = LV_KEY_END;
            break;
        default:
#if TARGET_WEBOS
            if (!read_webos_key(event, state)) {
                return false;
            }
#else
            return false;
#endif
    }
    state->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    return true;
}

#if TARGET_WEBOS

static bool read_webos_key(const SDL_KeyboardEvent *event, lv_drv_sdl_key_t *state) {
    switch ((int) event->keysym.scancode) {
        case SDL_WEBOS_SCANCODE_BACK:
            state->key = LV_KEY_ESC;
            return true;
        case SDL_WEBOS_SCANCODE_EXIT:
            if (!streaming_running()) {
                app_request_exit();
            }
            return false;
        case SDL_WEBOS_SCANCODE_RED:
        case SDL_WEBOS_SCANCODE_GREEN:
        case SDL_WEBOS_SCANCODE_YELLOW:
        case SDL_WEBOS_SCANCODE_BLUE: {
            SDL_Event btn_event;
            SDL_memcpy(&btn_event.key, event, sizeof(SDL_KeyboardEvent));
            btn_event.type = USER_REMOTEBUTTONEVENT;
            SDL_PushEvent(&btn_event);
            return false;
        }
        default:
            return false;
    }
}

static void webos_key_input_mode(const SDL_KeyboardEvent *event) {
    switch (event->keysym.sym) {
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT: {
            SDL_webOSCursorVisibility(SDL_FALSE);
            break;
        }
        default:
            break;
    }
}

#endif