#include "uuidstr.h"

#include <string.h>
#include <malloc.h>

void uuidstr_fromstr(uuidstr_t *dest, const char *src) {
    memcpy(dest->data, src, UUIDSTR_LENGTH);
    dest->zero = 0;
}

char *uuidstr_tostr(const uuidstr_t *src) {
    char *str = calloc(UUIDSTR_CAPACITY, sizeof(char));
    memcpy(str, src->data, UUIDSTR_LENGTH);
    return str;
}

bool uuidstr_t_equals_s(const uuidstr_t *a, const char *b) {
    return strncasecmp(a->data, b, UUIDSTR_LENGTH) == 0;
}

bool uuidstr_t_equals_t(const uuidstr_t *a, const uuidstr_t *b) {
    return strncasecmp(a->data, b->data, UUIDSTR_LENGTH) == 0;
}