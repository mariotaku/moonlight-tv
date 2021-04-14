#include "util/path.h"

#include <stdlib.h>
#include <string.h>

static const char kPathSeparator =
#if defined _WIN32 || defined __CYGWIN__
    '\\';
#else
    '/';
#endif

char *path_pref()
{
    char *path = getenv("HOME");
    if (!path)
        return "/tmp";
    path = strdup(path);
    int len = strlen(path);
    if (path[len - 1] == kPathSeparator)
    {
        path[len - 1] = '\0';
    }
    return path;
}