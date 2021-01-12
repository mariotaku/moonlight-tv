#include "path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *path_join(const char *parent, const char *basename)
{
    char *joined = calloc(strlen(parent) + 1 + strlen(basename) + 1, sizeof(char));
    sprintf(joined, "%s/%s", parent, basename);
    return joined;
}
