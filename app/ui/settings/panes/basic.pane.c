//
// Created by Mariotaku on 2021/08/30.
//

#include <malloc.h>
#include <lvgl.h>
#include <app.h>
#include "lvgl/ext/lv_obj_controller.h"
#include "pref_obj.h"

static void pane_ctor(lv_obj_controller_t *self, void *args);

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent);

const lv_obj_controller_class_t settings_pane_basic_cls = {
        .constructor_cb = pane_ctor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(lv_obj_controller_t),
};
static const pref_dropdown_int_pair_entry_t supported_resolutions[] = {
        {"720P",      1280, 720},
        {"1080P",     1920, 1080},
        {"1440P",     2560, 1440},
        {"4K",        3840, 2160},
        {"Automatic", 0,    0, true},
};

static const int supported_resolutions_len = sizeof(supported_resolutions) / sizeof(pref_dropdown_int_pair_entry_t);

static const pref_dropdown_int_entry_t supported_fps[] = {
        {"30 FPS",  30},
        {"60 FPS",  60, true},
        {"120 FPS", 120},
};
static const int supported_fps_len = sizeof(supported_fps) / sizeof(pref_dropdown_int_entry_t);

static void pane_ctor(lv_obj_controller_t *self, void *args) {

}

static lv_obj_t *create_obj(lv_obj_controller_t *self, lv_obj_t *parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_GRID);
    static const lv_coord_t col_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
                                         LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
    lv_obj_t *resolution_label = pref_title_label(parent, "Resolution and FPS");
    lv_obj_set_grid_cell(resolution_label, LV_GRID_ALIGN_STRETCH, 0, 2,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_t *resolution_dropdown = pref_dropdown_int_pair(parent, supported_resolutions, supported_resolutions_len,
                                                           &app_configuration->stream.width,
                                                           &app_configuration->stream.height);
    lv_obj_set_grid_cell(resolution_dropdown, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_t *fps_dropdown = pref_dropdown_int(parent, supported_fps, supported_fps_len,
                                               &app_configuration->stream.fps);
    lv_obj_set_grid_cell(fps_dropdown, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_t *bitrate_label = pref_title_label(parent, "Video bitrate");
    lv_obj_set_grid_cell(bitrate_label, LV_GRID_ALIGN_STRETCH, 0, 2,
                         LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_t *bitrate_slider = pref_slider(parent, &app_configuration->stream.bitrate, 5000, 60000, 1000);
    lv_obj_set_grid_cell(bitrate_slider, LV_GRID_ALIGN_STRETCH, 0, 2,
                         LV_GRID_ALIGN_CENTER, 3, 1);
    return NULL;
}