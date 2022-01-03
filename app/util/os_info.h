#pragma once

#define OS_VERSION_MAKE(X, Y, Z) ((X)*10000 + (Y)*100 + (Z))

typedef struct os_info_t {
    char name[32];
    char manufacturing_version[32];
    char release[32];
    int version;
} os_info_t;

int os_info_get(os_info_t *info);