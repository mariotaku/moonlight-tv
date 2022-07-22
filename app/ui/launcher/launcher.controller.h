#pragma once

#include <stdbool.h>
#include <lvgl/lv_sdl_img.h>

#include "ui/config.h"

#include "lvgl.h"

#include "backend/pcmanager.h"

typedef struct {
    lv_fragment_t base;
    lv_obj_t *nav;
    lv_obj_t *detail;
    lv_obj_t *pclist;
    lv_obj_t *add_btn, *pref_btn, *help_btn, *quit_btn;
    lv_group_t *nav_group, *detail_group;
    lv_style_transition_dsc_t tr_nav;
    lv_style_transition_dsc_t tr_detail;
    lv_style_t nav_host_style, nav_menu_style;
    lv_coord_t col_dsc[4], row_dsc[2];

    bool detail_opened;
    bool pane_initialized;
    bool first_created;
    bool detail_changing;
} launcher_controller_t;


lv_obj_t *launcher_win_create(lv_fragment_t *self, lv_obj_t *parent);

launcher_controller_t *launcher_instance();

void launcher_select_server(launcher_controller_t *controller, const char *uuid);

extern const lv_fragment_class_t launcher_controller_class;