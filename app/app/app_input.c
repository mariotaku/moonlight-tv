#include "app.h"

#include "lvgl/lv_sdl_drv_input.h"

#include "util/logging.h"

static void app_input_populate_group();

static SDL_Surface *blank_surface = NULL;
static SDL_Cursor *blank_cursor = NULL;

static lv_group_t *app_group = NULL;
static lv_ll_t modal_groups;

static lv_indev_t *app_indev_key, *app_indev_wheel, *app_indev_button, *app_indev_pointer;

static const lv_point_t button_points_empty[5] = {
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
};

void app_input_init() {
    blank_surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    blank_cursor = SDL_CreateColorCursor(blank_surface, 0, 0);
    if (!blank_cursor) {
        applog_w("Input", "Failed to create blank cursor: %s", SDL_GetError());
    }

    _lv_ll_init(&modal_groups, sizeof(lv_group_t *));
    lv_indev_t *indev_key = lv_sdl_init_key_input();
    lv_indev_t *indev_wheel = lv_sdl_init_wheel();
    lv_indev_t *indev_pointer = lv_sdl_init_pointer();
    lv_indev_t *indev_button = lv_sdl_init_button();
    app_indev_key = indev_key;
    app_indev_wheel = indev_wheel;
    app_indev_button = indev_button;
    app_indev_pointer = indev_pointer;
    lv_indev_set_button_points(indev_button, button_points_empty);
}

void app_input_deinit() {
    lv_sdl_deinit_button(app_indev_button);
    lv_sdl_deinit_pointer(app_indev_pointer);
    lv_sdl_deinit_wheel(app_indev_wheel);
    lv_sdl_deinit_key_input(app_indev_key);

    SDL_FreeCursor(blank_cursor);
    SDL_FreeSurface(blank_surface);
}

void app_input_set_group(lv_group_t *group) {
    app_group = group;
    app_input_populate_group();
}

lv_group_t *app_input_get_group() {
    return app_group;
}

void app_input_push_modal_group(lv_group_t *group) {
    LV_ASSERT_NULL(group);
    lv_group_t **tail = _lv_ll_ins_tail(&modal_groups);
    *tail = group;
    app_input_populate_group();
}

void app_input_remove_modal_group(lv_group_t *group) {
    lv_group_t **node = NULL;
    _LV_LL_READ_BACK(&modal_groups, node) {
        if (*node == group) break;
    }
    if (node) {
        _lv_ll_remove(&modal_groups, node);
        lv_mem_free(node);
    }
    app_input_populate_group();
}

void app_input_set_button_points(const lv_point_t *points) {
    lv_indev_set_button_points(app_indev_button, points ? points : button_points_empty);
}


void app_start_text_input(int x, int y, int w, int h) {
    if (w > 0 && h > 0) {
        struct SDL_Rect rect = {x, y, w, h};
        SDL_SetTextInputRect(&rect);
    } else {
        SDL_SetTextInputRect(NULL);
    }
    lv_sdl_key_input_release_key(app_indev_key);
    if (SDL_IsTextInputActive()) return;
    SDL_StartTextInput();
}

void app_stop_text_input() {
    SDL_StopTextInput();
}

bool app_text_input_active() {
    return SDL_IsTextInputActive();
}

void app_input_inject_key(lv_key_t key, bool pressed) {
    lv_sdl_key_input_inject_key(app_indev_key, key, pressed);
}

void app_set_mouse_grab(bool grab) {
#if HAVE_RELATIVE_MOUSE_HACK
    if (grab) {
        applog_d("Input", "Set cursor to blank bitmap: %p", blank_cursor);
        SDL_SetCursor(blank_cursor);
    } else {
        SDL_SetCursor(SDL_GetDefaultCursor());
    }
#else
    if (app_configuration->hardware_mouse) {
        SDL_ShowCursor(grab ? SDL_FALSE : SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(grab && !app_configuration->absmouse ? SDL_TRUE : SDL_FALSE);
        if (!grab) {
            SDL_ShowCursor(SDL_TRUE);
        }
    }
#endif
}

bool app_get_mouse_relative() {
#if HAVE_RELATIVE_MOUSE_HACK
    return !app_configuration->absmouse;
#else
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
#endif
}

static void app_input_populate_group() {
    lv_group_t *group = NULL;
    lv_group_t *const *tail = _lv_ll_get_tail(&modal_groups);
    if (tail) {
        group = *tail;
    }
    if (!group) {
        group = app_group;
    }
    if (!group) {
        group = lv_group_get_default();
    }
    lv_indev_set_group(app_indev_key, group);
    lv_indev_set_group(app_indev_wheel, group);
    lv_indev_set_group(app_indev_button, group);
}