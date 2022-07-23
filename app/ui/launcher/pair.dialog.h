#pragma once

#include "backend/types.h"

#include "lvgl.h"

extern const lv_fragment_class_t pair_dialog_class;

void pair_dialog_open(const SERVER_LIST *node);