#pragma once

#include "app_input.h"

typedef struct executor_t executor_t;
typedef struct app_settings_t app_settings_t;

void app_input_init_gamepad_mapping(app_input_t *input, executor_t *executor, const app_settings_t *settings);

void app_input_deinit_gamepad_mapping(app_input_t *input);

void app_input_copy_initial_gamepad_mapping(const app_settings_t *settings);