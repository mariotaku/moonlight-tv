#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lvgl.h>

#include "app.h"

#include "ui/config.h"
#include "util/navkey.h"
#include "os_info.h"

#include "stream/settings.h"

#if TARGET_WEBOS

#include "panel_info.h"

#endif

typedef struct app_t app_t;

typedef struct {
    lv_fragment_t base;
    app_t *app;

    bool mini, pending_mini;

    lv_obj_t *nav;
    lv_group_t *nav_group;

    lv_obj_t *detail;
    lv_group_t *detail_group;

    lv_obj_t *tabview;
    lv_group_t **tab_groups;

    lv_obj_t *close_btn;

    lv_obj_t *active_dropdown;

    os_info_t os_info;
    bool needs_restart;
#if TARGET_WEBOS
    webos_panel_info_t webos_panel_info;
#endif
} settings_controller_t;

lv_obj_t *settings_win_create(lv_fragment_t *self, lv_obj_t *parent);

extern const lv_fragment_class_t settings_controller_cls;
extern const lv_fragment_class_t settings_pane_basic_cls;
extern const lv_fragment_class_t settings_pane_host_cls;
extern const lv_fragment_class_t settings_pane_input_cls;
extern const lv_fragment_class_t settings_pane_decoder_cls;
extern const lv_fragment_class_t settings_pane_about_cls;