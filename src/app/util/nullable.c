//
// Created by Mariotaku on 2021/09/14.
//

#include "nullable.h"
#include <stdlib.h>
#include <string.h>

void free_nullable(void *p) {
    if (!p) { return; }
    free(p);
}

char *strdup_nullable(const char *p) {
    if (!p) { return NULL; }
    return strdup(p);
}

bool str_null_or_empty(const char *p) {
    if (p == NULL) { return true; }
    return *p == '\0';
}