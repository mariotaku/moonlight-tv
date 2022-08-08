#include "app.h"
#include "lvgl/lv_sdl_drv_input.h"

static void app_input_populate_group();

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