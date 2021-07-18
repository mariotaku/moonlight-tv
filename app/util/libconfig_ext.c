#include "libconfig_ext.h"

#include <dlfcn.h>
#include <string.h>

int config_setting_set_enum(config_setting_t *setting, int value, const char *(*converter)(int))
{
    return config_setting_set_string(setting, converter(value));
}

int config_lookup_enum(const config_t *config, const char *path, int *value, int (*converter)(const char *))
{
    int ret;
    const char *str = NULL;
    if ((ret = config_lookup_string(config, path, &str)) == CONFIG_TRUE)
    {
        *value = converter(str);
    }
    return ret;
}

int config_lookup_string_dup(const config_t *config, const char *path, const char **value)
{
    int ret;
    if ((ret = config_lookup_string(config, path, value)) == CONFIG_TRUE && *value)
    {
        *value = strdup(*value);
    }
    return ret;
}

const char *config_setting_get_string_simple(config_setting_t *setting, const char *name)
{
    config_setting_t *member = config_setting_get_member(setting, name);
    if (!member)
        return NULL;
    return config_setting_get_string(member);
}

void config_setting_set_string_simple(config_setting_t *parent, const char *key, const char *value)
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_STRING);
    config_setting_set_string(setting, value);
}

void config_setting_set_int_simple(config_setting_t *parent, const char *key, int value)
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_INT);
    config_setting_set_int(setting, value);
}

bool config_setting_get_bool_simple(config_setting_t *setting, const char *name)
{
    config_setting_t *member = config_setting_get_member(setting, name);
    if (!member)
        return false;
    return config_setting_get_bool(member);
}

void config_setting_set_bool_simple(config_setting_t *parent, const char *key, bool value)
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_BOOL);
    config_setting_set_bool(setting, value);
}

int config_setting_set_enum_simple(config_setting_t *parent, const char *key, int value, const char *(*converter)(int))
{
    config_setting_t *setting = config_setting_add(parent, key, CONFIG_TYPE_STRING);
    return config_setting_set_enum(setting, value, converter);
}

int config_get_options(const config_t *config)
{
    int (*fn)(config_t *) = dlsym(RTLD_NEXT, "config_get_options");
    if (!fn)
        return 0;
    return fn(config);
}

void config_set_options(config_t *config, int options)
{
    void (*fn)(config_t *, int) = dlsym(RTLD_NEXT, "config_set_options");
    if (!fn)
        return;
    fn(config, options);
}