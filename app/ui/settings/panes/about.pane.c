#include "app.h"
#include <stdlib.h>
#include <lvgl.h>
#include "stream/platform.h"
#include "pref_obj.h"
#include "ui/settings/settings.controller.h"

#include "util/i18n.h"
#include "os_info.h"

#if TARGET_WEBOS

#include <SDL_webOS.h>

#endif

#define MAXIMUM_ROWS 32

typedef struct about_pane_t {
    lv_fragment_t base;
    settings_controller_t *parent;

    lv_coord_t row_dsc[MAXIMUM_ROWS + 1];
#if TARGET_WEBOS
    struct {
        int w, h;
        int rate;
    } webos_panel_info;
#endif
} about_pane_t;

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void pane_ctor(lv_fragment_t *self, void *args);

#if TARGET_WEBOS

static void load_webos_info(about_pane_t *controller);

#endif

const lv_fragment_class_t settings_pane_about_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(about_pane_t),
};

static void pane_ctor(lv_fragment_t *self, void *args) {
    about_pane_t *controller = (about_pane_t *) self;
    controller->parent = args;
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

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    about_pane_t *controller = (about_pane_t *) self;
    app_t *app = controller->parent->app;

    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_GRID);
    static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                         LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(view, col_dsc, controller->row_dsc);
    int rowcount = 0;
    about_line(view, locstr("Version"), APP_VERSION, rowcount++, 1);
    about_line(view, locstr("Video decoder"), SS4S_ModuleInfoGetId(app->ss4s.selection.video_module), rowcount++, 2);
    about_line(view, locstr("Audio backend"), SS4S_ModuleInfoGetId(app->ss4s.selection.audio_module), rowcount++, 2);
#if TARGET_WEBOS
    const os_info_t *os_info = &controller->parent->os_info;
    char *version_name = version_info_str(&os_info->version);
    if (version_name != NULL) {
        about_line(view, locstr("webOS version"), version_name, rowcount++, 1);
    }
    free(version_name);
    if (controller->webos_panel_info.h && controller->webos_panel_info.w) {
        char resolution_text[16];
        SDL_snprintf(resolution_text, sizeof(resolution_text), "%5d * %5d", controller->webos_panel_info.w,
                     controller->webos_panel_info.h);
        about_line(view, locstr("Screen resolution"), resolution_text, rowcount++, 1);
    }
    if (controller->webos_panel_info.rate) {
        char fps_text[16];
        SDL_snprintf(fps_text, sizeof(fps_text), "%4dFPS", controller->webos_panel_info.rate);
        about_line(view, locstr("Refresh rate"), fps_text, rowcount++, 1);
    }
#endif
    LV_ASSERT(rowcount <= MAXIMUM_ROWS);
    for (int i = 0; i < rowcount; i++) {
        controller->row_dsc[i] = LV_GRID_CONTENT;
    }
    controller->row_dsc[rowcount] = LV_GRID_TEMPLATE_LAST;

    return view;
}

#if TARGET_WEBOS

static void load_webos_info(about_pane_t *controller) {
    SDL_webOSGetPanelResolution(&controller->webos_panel_info.w, &controller->webos_panel_info.h);
    SDL_webOSGetRefreshRate(&controller->webos_panel_info.rate);
}

#endif