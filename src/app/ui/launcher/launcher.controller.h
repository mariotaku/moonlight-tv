#pragma once

#include <stdbool.h>
#include "ui/config.h"

#include "lvgl.h"
#include "lv_sdl_img.h"

#include "backend/pcmanager.h"

typedef struct app_t app_t;
typedef struct app_launch_params_t app_launch_params_t;

typedef struct launcher_fragment_args_t {
    app_t *app;
    const app_launch_params_t *params;
} launcher_fragment_args_t;

typedef struct launcher_fragment_t {
    lv_fragment_t base;

    app_t *global;

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

    const app_launch_params_t *launch_params;
    bool def_host_selected;
    bool def_app_requested;
} launcher_fragment_t;


lv_obj_t *launcher_win_create(lv_fragment_t *self, lv_obj_t *parent);

launcher_fragment_t *launcher_instance();

void launcher_select_server(launcher_fragment_t *controller, const uuidstr_t *uuid);

extern const lv_fragment_class_t launcher_controller_class;