#pragma once

#include <backend/apploader/apploader.h>
#include "lvgl.h"
#include "client.h"

#include "lvgl/lv_sdl_img.h"
#include "apps.controller.h"

lv_obj_t *appitem_view(apps_controller_t *controller, lv_obj_t *parent);

void appitem_style_init(appitem_styles_t *style);

void appitem_style_deinit(appitem_styles_t *style);