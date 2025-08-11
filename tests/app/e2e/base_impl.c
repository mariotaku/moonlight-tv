#include <stdlib.h>
#include <stdio.h>

#include "e2e_base.h"
#include "uuidstr.h"

static int argc = 1;
static char *argv[] = {"moonlight"};

app_t app;

extern const lv_img_dsc_t e2e_lv_cursor_img;

static int settingsLoader(app_settings_t *settings) {
    char *path = malloc(128);
    uuidstr_t uuid;
    uuidstr_random(&uuid);
    snprintf(path, 128, "/tmp/moonlight-test-%s", (char *) &uuid);
    settings_initialize(settings, path);
    app.ui.dpi = 320;
    initSettings(settings);
    return 0;
}

static void cursor_style_cb(lv_event_t *e) {
    lv_obj_t *cursor = lv_event_get_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "%d,%d", lv_obj_get_style_x(cursor, 0), lv_obj_get_style_y(cursor, 0));
}

void setUp(void) {
    app_init(&app, settingsLoader, argc, argv);
    app_ui_open(&app.ui, false, NULL);

    lv_obj_t *cursor = lv_obj_create(lv_disp_get_scr_act(NULL));
    lv_obj_remove_style_all(cursor);
    lv_obj_set_flex_flow(cursor, LV_FLEX_FLOW_ROW);

    lv_obj_t *cursor_img = lv_img_create(cursor);
    lv_img_set_src(cursor_img, &e2e_lv_cursor_img);

    lv_obj_t *cursor_label = lv_label_create(cursor);
    lv_label_set_text(cursor_label, "");
    lv_obj_set_style_text_font(cursor_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(cursor_label, lv_color_white(), 0);
    lv_obj_set_style_bg_color(cursor_label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(cursor_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(cursor_label, LV_DPX(2), 0);
    lv_obj_add_event_cb(cursor, cursor_style_cb, LV_EVENT_STYLE_CHANGED, cursor_label);

    lv_indev_set_cursor(app.ui.input.pointer.indev, cursor);
}

void tearDown(void) {
    if (SDL_GetHintBoolean("TEST_E2E_WAIT_FOR_EXIT", SDL_FALSE)) {
        while (app.running) {
            app_run_loop(&app);
        }
    } else {
        app_request_exit();
    }
    app_deinit(&app);
}

void waitFor(int ms) {
    int start = (int) SDL_GetTicks();
    while (app.running && (SDL_GetTicks() - start) < ms) {
        app_run_loop(&app);
    }
}

void fakeTap(int x, int y) {
    SDL_Event event = {
        .motion = {
            .type = SDL_MOUSEMOTION,
            .timestamp = SDL_GetTicks(),
            .x = x,
            .y = y,
        },
    };
    SDL_PushEvent(&event);
    waitFor(20);
    event.button = (SDL_MouseButtonEvent) {
        .type = SDL_MOUSEBUTTONDOWN,
        .timestamp = SDL_GetTicks(),
        .button = SDL_BUTTON_LEFT,
        .state = SDL_PRESSED,
        .x = x,
        .y = y,
    };
    SDL_PushEvent(&event);
    waitFor(20);
    event.button = (SDL_MouseButtonEvent) {
        .type = SDL_MOUSEBUTTONUP,
        .timestamp = SDL_GetTicks(),
        .button = SDL_BUTTON_LEFT,
        .state = SDL_RELEASED,
        .x = x,
        .y = y,
    };
    SDL_PushEvent(&event);
    waitFor(20);
}

void fakeKeyPress(SDL_Keycode key) {
    SDL_Event event = {
        .key = {
            .type = SDL_KEYDOWN,
            .timestamp = SDL_GetTicks(),
            .keysym = {
                .sym = key,
                .mod = 0,
                .scancode = SDL_GetScancodeFromKey(key),
            },
        },
    };
    SDL_PushEvent(&event);
    waitFor(20);

    event.key.type = SDL_KEYUP;
    SDL_PushEvent(&event);
    waitFor(20);
}

void fakeInput(const char *text) {
    SDL_Event event = {
        .text = {
            .type = SDL_TEXTINPUT,
            .timestamp = SDL_GetTicks(),
        },
    };
    strncpy(event.text.text, text, SDL_TEXTINPUTEVENT_TEXT_SIZE - 1);
    SDL_PushEvent(&event);
    waitFor(20);
}