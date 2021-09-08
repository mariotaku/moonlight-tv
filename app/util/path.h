#pragma once

#include <stddef.h>

char *path_join(const char *parent, const char *basename);

void path_join_to(char *dest, size_t maxlen, const char *parent, const char *basename);

char *path_pref();