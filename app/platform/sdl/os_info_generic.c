#include <string.h>

#include <SDL.h>
#include "util/os_info.h"

int os_info_get(os_info_t *info) {
    memset(info, 0, sizeof(os_info_t));
    strncpy(info->name, SDL_GetPlatform(), sizeof(info->name) - 1);
    return 0;
}