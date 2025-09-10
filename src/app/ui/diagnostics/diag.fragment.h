#pragma once

#include "lvgl.h"
#include "executor.h"

typedef struct diag_fragment_t {
    lv_fragment_t base;

    lv_style_t diag_item_style;
    lv_style_t diag_spinner_style;
    lv_style_t diag_title_style;
    lv_style_t hint_item_style;
    lv_style_t large_iconfont_style;

    executor_t *executor;
} diag_fragment_t;

extern const lv_fragment_class_t diag_fragment_class;