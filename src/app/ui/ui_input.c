#include "root.h"
#include "ui_input.h"
#include "lvgl/input/lv_drv_sdl_key.h"
#include "lvgl/lv_sdl_drv_input.h"

static void app_input_populate_group(app_ui_input_t *input);


static const lv_point_t button_points_empty[5] = {
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
        {.x= 0, .y = 0},
};


void app_ui_input_init(app_ui_input_t *input, app_ui_t *ui) {
    input->ui = ui;
    _lv_ll_init(&input->modal_groups, sizeof(lv_group_t *));
    lv_group_t *group = lv_group_get_default();

    lv_sdl_init_key_input(&input->key.drv, input);
    lv_sdl_init_pointer(&input->pointer.drv, input);
    lv_sdl_init_wheel(&input->wheel.drv, input);
    lv_sdl_init_button(&input->button.drv, input);

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

void app_ui_input_deinit(app_ui_input_t *input) {
    _lv_ll_clear(&input->modal_groups);
}

void app_input_set_group(app_ui_input_t *input, lv_group_t *group) {
    input->app_group = group;
    app_input_populate_group(input);
}

lv_group_t *app_input_get_group(app_ui_input_t *input) {
    return input->app_group;
}

void app_input_push_modal_group(app_ui_input_t *input, lv_group_t *group) {
    LV_ASSERT_NULL(group);
    lv_group_t **tail = _lv_ll_ins_tail(&input->modal_groups);
    *tail = group;
    app_input_populate_group(input);
}

void app_input_remove_modal_group(app_ui_input_t *input, lv_group_t *group) {
    lv_group_t **node = NULL;
    _LV_LL_READ_BACK(&input->modal_groups, node) {
        if (*node == group) { break; }
    }
    if (node) {
        _lv_ll_remove(&input->modal_groups, node);
        lv_mem_free(node);
    }
    app_input_populate_group(input);
}


void app_input_set_button_points(app_ui_input_t *input, const lv_point_t *points) {
    lv_indev_set_button_points(input->button.indev, points ? points : button_points_empty);
}


void app_start_text_input(app_ui_input_t *input, int x, int y, int w, int h) {
    if (w > 0 && h > 0) {
        struct SDL_Rect rect = {x, y, w, h};
        SDL_SetTextInputRect(&rect);
    } else {
        SDL_SetTextInputRect(NULL);
    }
    lv_sdl_key_input_release_key(input->key.indev);
    if (SDL_IsTextInputActive()) { return; }
    SDL_StartTextInput();
}

static void app_input_populate_group(app_ui_input_t *input) {
    lv_group_t *group = NULL;
    lv_group_t *const *tail = _lv_ll_get_tail(&input->modal_groups);
    if (tail) {
        group = *tail;
    }
    if (!group) {
        group = input->app_group;
    }
    if (!group) {
        group = lv_group_get_default();
    }
    lv_indev_set_group(input->key.indev, group);
    lv_indev_set_group(input->wheel.indev, group);
    lv_indev_set_group(input->button.indev, group);
}