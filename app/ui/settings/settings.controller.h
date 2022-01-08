#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/config.h"
#include "util/navkey.h"

#include "lvgl.h"

#include "stream/settings.h"
#include "app.h"

typedef struct {
    lv_fragment_t base;
    bool mini, pending_mini;

    lv_obj_t *nav;
    lv_group_t *nav_group;

    lv_obj_t *detail;
    lv_group_t *detail_group;

    lv_obj_t *tabview;
    lv_group_t **tab_groups;

    lv_obj_t *close_btn;

    lv_obj_t *active_dropdown;
    bool needs_restart;
} settings_controller_t;

lv_obj_t *settings_win_create(struct lv_fragment_t *self, lv_obj_t *parent);

extern const lv_fragment_class_t settings_controller_cls;
extern const lv_fragment_class_t settings_pane_basic_cls;
extern const lv_fragment_class_t settings_pane_host_cls;
extern const lv_fragment_class_t settings_pane_input_cls;
extern const lv_fragment_class_t settings_pane_decoder_cls;
extern const lv_fragment_class_t settings_pane_about_cls;