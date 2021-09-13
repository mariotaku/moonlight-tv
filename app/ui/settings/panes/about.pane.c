//
// Created by Mariotaku on 2021/08/30.
//

#include <malloc.h>
#include <lvgl.h>
#include <app.h>
#include <stream/platform.h>
#include "lvgl/lv_obj_controller.h"
#include "pref_obj.h"

#if TARGET_WEBOS
#include <SDL_webOS.h>
#include "platform/webos/os_info.h"
#endif

#define MAXIMUM_ROWS 32

typedef struct about_pane_t {
    lv_obj_controller_t base;
    lv_coord_t row_dsc[MAXIMUM_ROWS + 1];
#if TARGET_WEBOS
    char webos_release[32];
    struct {
        int w, h;
        int rate;
    } webos_panel_info;
#endif
} about_pane_t;

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent);

static void pane_ctor(lv_obj_controller_t *self, void *args);

#if TARGET_WEBOS
static void load_webos_info(about_pane_t* controller);
#endif

const lv_obj_controller_class_t settings_pane_about_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(about_pane_t),
};

static void pane_ctor(lv_obj_controller_t *self, void *args) {
    about_pane_t *controller = (about_pane_t *) self;
#if TARGET_WEBOS
    load_webos_info(controller);
#endif
}

static inline void about_line(lv_obj_t *parent, const char *title, const char *text, int row, int text_span) {
    LV_ASSERT(text_span < 4);
    lv_obj_t *title_label = lv_label_create(parent);
    lv_obj_t *text_label = lv_label_create(parent);
    lv_label_set_text(title_label, title);
    lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 0, 4 - text_span, LV_GRID_ALIGN_CENTER, row, 1);
    lv_label_set_text(text_label, text);
    lv_obj_set_grid_cell(text_label, LV_GRID_ALIGN_END, 4 - text_span, text_span, LV_GRID_ALIGN_CENTER, row, 1);
}

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent) {
    about_pane_t *controller = (about_pane_t *) self;
    lv_obj_set_layout(parent, LV_LAYOUT_GRID);
    static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                         LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, col_dsc, controller->row_dsc);
    int rowcount = 0;
    about_line(parent, "Version", APP_VERSION, rowcount++, 1);
    about_line(parent, "Decoder module", decoder_definitions[decoder_current].name, rowcount++, 2);
    const char *audio_name = audio_current == AUDIO_DECODER ? "Use decoder" : audio_definitions[audio_current].name;
    about_line(parent, "Audio backend", audio_name, rowcount++, 2);
#if TARGET_WEBOS
    about_line(parent, "webOS version", "none", rowcount++, 1);
    about_line(parent, "Firmware version", "none", rowcount++, 1);
    about_line(parent, "Panel resolution", "none", rowcount++, 1);
    about_line(parent, "Refresh rate", "none", rowcount++, 1);
#endif
    LV_ASSERT(rowcount <= MAXIMUM_ROWS);
    for (int i = 0; i < rowcount; i++) {
        controller->row_dsc[i] = LV_GRID_CONTENT;
    }
    controller->row_dsc[rowcount] = LV_GRID_TEMPLATE_LAST;

    return NULL;
}

#if TARGET_WEBOS
static void load_webos_info(about_pane_t* controller) {
    webos_os_info_get_release(controller->webos_release, sizeof(controller->webos_release));
    SDL_webOSGetPanelResolution(&webos_panel_info.w, &webos_panel_info.h);
    SDL_webOSGetRefreshRate(&webos_panel_info.rate);
}
#endif