#include "path.h"

#include <stdio.h>
#include <string.h>

#include "util/memlog.h"

#if __WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

char *path_join(const char *parent, const char *basename) {
    int parentlen = strlen(parent);
    if (parentlen && parent[parentlen - 1] == PATH_SEPARATOR) {
        parentlen -= 1;
    }
    char *joined = calloc(parentlen + 1 + strlen(basename) + 1, sizeof(char));
    sprintf(joined, "%.*s%c%s", parentlen, parent, PATH_SEPARATOR, basename);
    return joined;
}
