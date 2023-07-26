#pragma once

#include <stdbool.h>

void free_nullable(void *p);

char *strdup_nullable(const char *p);

bool str_null_or_empty(const char *p);
