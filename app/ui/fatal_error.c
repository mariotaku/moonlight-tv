#include "fatal_error.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "lvgl.h"

#include "util/bus.h"

static void fatal_error_popup(void *data);

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
    bus_pushaction(fatal_error_popup, data);
}

_Noreturn void app_halt() {
    while (1);
}

static void fatal_error_popup(void *data) {
    fatal_error_data_t *error_data = data;
    static const char *btn_txts[] = {"Quit", ""};
    lv_obj_t *msgbox = lv_msgbox_create(NULL, error_data->title, error_data->message, btn_txts, false);
    lv_obj_add_event_cb(msgbox, (lv_event_cb_t) abort, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(msgbox);
}