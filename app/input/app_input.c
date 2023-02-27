#include "app_input.h"
#include "app.h"

#include "lvgl/lv_sdl_drv_input.h"

#include "util/logging.h"

static void app_input_populate_group(app_input_t *input);

static SDL_Surface *blank_surface = NULL;
static SDL_Cursor *blank_cursor = NULL;

static lv_group_t *app_group = NULL;
static lv_ll_t modal_groups;

static const lv_point_t button_points_empty[5] = {
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
};

void app_input_init(app_input_t *input, app_t *app) {
    blank_surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    blank_cursor = SDL_CreateColorCursor(blank_surface, 0, 0);
    if (!blank_cursor) {
        applog_w("Input", "Failed to create blank cursor: %s", SDL_GetError());
    }

    _lv_ll_init(&modal_groups, sizeof(lv_group_t *));
    lv_group_t *group = lv_group_get_default();

    lv_sdl_init_key_input(&input->key.drv);
    lv_sdl_init_pointer(&input->pointer.drv);
    lv_sdl_init_wheel(&input->wheel.drv);
    lv_sdl_init_button(&input->button.drv);
    input->pointer.drv.user_data = app;

    input->key.indev = lv_indev_drv_register(&input->key.drv.base);
    input->pointer.indev = lv_indev_drv_register(&input->pointer.drv);
    input->wheel.indev = lv_indev_drv_register(&input->wheel.drv);
    input->button.indev = lv_indev_drv_register(&input->button.drv);

    if (group) {
        lv_indev_set_group(input->key.indev, group);
        lv_indev_set_group(input->wheel.indev, group);
        lv_indev_set_group(input->button.indev, group);
    }

    lv_indev_set_button_points(input->button.indev, button_points_empty);
}

void app_input_deinit(app_input_t *input) {

    SDL_FreeCursor(blank_cursor);
    SDL_FreeSurface(blank_surface);
}

void app_input_set_group(app_input_t *input, lv_group_t *group) {
    app_group = group;
    app_input_populate_group(input);
}

lv_group_t *app_input_get_group(app_input_t *input) {
    return app_group;
}

void app_input_push_modal_group(app_input_t *input, lv_group_t *group) {
    LV_ASSERT_NULL(group);
    lv_group_t **tail = _lv_ll_ins_tail(&modal_groups);
    *tail = group;
    app_input_populate_group(input);
}

void app_input_remove_modal_group(app_input_t *input, lv_group_t *group) {
    lv_group_t **node = NULL;
    _LV_LL_READ_BACK(&modal_groups, node) {
        if (*node == group) break;
    }
    if (node) {
        _lv_ll_remove(&modal_groups, node);
        lv_mem_free(node);
    }
    app_input_populate_group(input);
}

void app_input_set_button_points(app_input_t *input, const lv_point_t *points) {
    lv_indev_set_button_points(input->button.indev, points ? points : button_points_empty);
}


void app_start_text_input(app_input_t *input, int x, int y, int w, int h) {
    if (w > 0 && h > 0) {
        struct SDL_Rect rect = {x, y, w, h};
        SDL_SetTextInputRect(&rect);
    } else {
        SDL_SetTextInputRect(NULL);
    }
    lv_sdl_key_input_release_key(input->key.indev);
    if (SDL_IsTextInputActive()) return;
    SDL_StartTextInput();
}

void app_stop_text_input(app_input_t *input) {
    SDL_StopTextInput();
}

bool app_text_input_active(app_input_t *input) {
    return SDL_IsTextInputActive();
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

static void app_input_populate_group(app_input_t *input) {
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
    lv_indev_set_group(input->key.indev, group);
    lv_indev_set_group(input->wheel.indev, group);
    lv_indev_set_group(input->button.indev, group);
}