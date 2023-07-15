#include "fatal_error.h"
#include "app.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <SDL_thread.h>

#include "lvgl.h"

#include "util/bus.h"
#include "logging.h"

static void fatal_error_popup(void *data);

static void fatal_error_quit(lv_event_t *ev);

typedef struct fatal_error_data_t {
    char title[32];
    char message[1024];
} fatal_error_data_t;

void app_fatal_error(const char *title, const char *fmt, ...) {
    fatal_error_data_t *data = malloc(sizeof(fatal_error_data_t));
    snprintf(data->title, 32, "%s", title);
    va_list args;
    va_start(args, fmt);
    vsnprintf(data->message, 1024, fmt, args);
    va_end(args);
    app_bus_post(global, fatal_error_popup, data);
}


static void fatal_error_popup(void *data) {
    fatal_error_data_t *error_data = data;
    static const char *btn_txts[] = {"Quit", ""};
    lv_obj_t *msgbox = lv_msgbox_create(NULL, error_data->title, error_data->message, btn_txts, false);
    lv_obj_add_event_cb(msgbox, fatal_error_quit, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(msgbox, LV_OBJ_FLAG_USER_4);
    lv_obj_center(msgbox);
}

static void fatal_error_quit(lv_event_t *ev) {
    exit(1);
}