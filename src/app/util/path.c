#include "path.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>

#include "util/compat.h"

char *path_join(const char *parent, const char *basename) {
    unsigned int parentlen = strlen(parent);
    if (parentlen && parent[parentlen - 1] == PATH_SEPARATOR) {
        parentlen -= 1;
    }
    char *joined = calloc(parentlen + 1 + strlen(basename) + 1, sizeof(char));
    sprintf(joined, "%.*s%c%s", parentlen, parent, PATH_SEPARATOR, basename);
    return joined;
}

void path_join_to(char *dest, size_t maxlen, const char *parent, const char *basename) {
    unsigned int parentlen = strlen(parent);
    if (parentlen && parent[parentlen - 1] == PATH_SEPARATOR) {
        parentlen -= 1;
    }
    snprintf(dest, maxlen, "%.*s%c%s", parentlen, parent, PATH_SEPARATOR, basename);
}

int path_dir_ensure(const char *dir) {
    if (access(dir, F_OK | W_OK) == 0) {
        return 0;
    }
    if (errno == ENOENT) {
        char tmp[PATH_MAX];
        char *last_slash = strrchr(dir, PATH_SEPARATOR);
        if (last_slash && last_slash != dir) {
            strncpy(tmp, dir, last_slash - dir);
            tmp[last_slash - dir] = '\0';
            if (path_dir_ensure(tmp) == -1) {
                return -1;
            }
        }
        return MKDIR(dir, 0755);
    }
    return -1;
}