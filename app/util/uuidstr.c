#include "uuidstr.h"

#include <string.h>

void uuidstr_fromstr(uuidstr_t *dest, const char *src) {
    memcpy(dest->data, src, UUIDSTR_LENGTH);
    dest->zero = 0;
}

bool uuidstr_strequals(const char *a, const char *b) {
    return strncasecmp(a, b, UUIDSTR_LENGTH) == 0;
}

bool uuidstr_t_equals_s(const uuidstr_t *a, const char *b) {
    return strncasecmp(a->data, b, UUIDSTR_LENGTH) == 0;
}

bool uuidstr_t_equals_t(const uuidstr_t *a, const uuidstr_t *b) {
    return strncasecmp(a->data, b->data, UUIDSTR_LENGTH) == 0;
}