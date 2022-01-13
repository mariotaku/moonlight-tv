#include <string.h>
#include "util/os_info.h"

int os_info_get(os_info_t *info) {
    memset(info, 0, sizeof(os_info_t));
    strcpy(info->name, "Windows");
    strcpy(info->release, "0");
    strcpy(info->manufacturing_version, "0");
    return 0;
}