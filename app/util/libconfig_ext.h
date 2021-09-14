#pragma once

#include <stdbool.h>
#include <libconfig.h>


int config_setting_set_enum(config_setting_t *setting, int value, const char *(*converter)(int));

int config_lookup_enum(const config_t *config, const char *path, int *value, int (*converter)(const char *));

int config_lookup_string_dup(const config_t *config, const char *path, char **value);

const char *config_setting_get_string_simple(config_setting_t *setting, const char *name);

int config_setting_set_string_simple(config_setting_t *parent, const char *key, const char *value);

int config_setting_set_int_simple(config_setting_t *parent, const char *key, int value);

bool config_setting_get_bool_simple(config_setting_t *setting, const char *name);

int config_setting_set_bool_simple(config_setting_t *parent, const char *key, bool value);

int config_setting_set_enum_simple(config_setting_t *parent, const char *key, int value, const char *(*converter)(int));

int config_lookup_bool_std(const config_t *config, const char *path, bool *value);