#include "path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *path_join(const char *parent, const char *basename)
{
    int parentlen = strlen(parent);
    if (parentlen && parent[parentlen - 1] == '/')
    {
        parentlen -= 1;
    }
    char *joined = calloc(parentlen + 1 + strlen(basename) + 1, sizeof(char));
    sprintf(joined, "%.*s/%s", parentlen, parent, basename);
    return joined;
}
