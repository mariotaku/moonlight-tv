#pragma once

#include <stddef.h>

typedef struct webos_os_info_t {
    char manufacturing_version[32];
    char release[32];
} webos_os_info_t;

int webos_os_info_get_release(webos_os_info_t *info);