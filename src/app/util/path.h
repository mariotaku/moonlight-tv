#pragma once

#include <stddef.h>

#if __WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

char *path_join(const char *parent, const char *basename);

void path_join_to(char *dest, size_t maxlen, const char *parent, const char *basename);

char *path_assets();

char *path_pref();

char *path_cache();

int path_dir_ensure(const char *dir);