#pragma once

#include "lvgl.h"
#include "lvgl/ext/lv_obj_controller.h"
#include "stream/module/api.h"
#include "stream/settings.h"
#include "backend/pcmanager.h"
#include "libgamestream/client.h"

extern FILE *app_logfile;
extern PCONFIGURATION app_configuration;
extern lv_controller_manager_t *app_uimanager;
extern pcmanager_t *pcmanager;

int app_init(int argc, char *argv[]);

void app_process_events();

void app_request_exit();

void app_quit_confirm();

void app_start_text_input(int x, int y, int w, int h);

void app_stop_text_input();

bool ui_render_queue_submit(void *data, unsigned int pts);

GS_CLIENT app_gs_client_new();

void app_set_mouse_grab(bool);

void app_set_keep_awake(bool);

void app_input_set_group(lv_group_t *group);

lv_group_t *app_input_get_group();

void app_set_fullscreen(bool);