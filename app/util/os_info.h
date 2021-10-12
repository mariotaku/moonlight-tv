#pragma once

typedef struct os_info_t {
    char name[32];
    char manufacturing_version[32];
    char release[32];
} os_info_t;

int os_info_get(os_info_t *info);