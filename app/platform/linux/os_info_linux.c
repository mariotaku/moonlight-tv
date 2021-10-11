#include <string.h>
#include <stdio.h>
#include "util/os_info.h"

int os_info_get(os_info_t *info) {
    memset(info, 0, sizeof(os_info_t));
    FILE *fp = popen("lsb_release -s -i", "r");
    if (fp != NULL && fgets(info->name, sizeof(info->name), fp)) {
        info->name[strlen(info->name) - 1] = '\0';
    } else {
        strncpy(info->name, "Linux", sizeof(info->name));
    }
    if (fp) {
        fclose(fp);
    }
    fp = popen("lsb_release -s -r", "r");
    if (fp != NULL && fgets(info->release, sizeof(info->release), fp)) {
        info->release[strlen(info->release) - 1] = '\0';
    } else {
        strncpy(info->release, "unknown", sizeof(info->release));
    }
    if (fp) {
        fclose(fp);
    }
    return 0;
}