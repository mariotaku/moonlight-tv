#include "uuidstr.h"

#include <string.h>

void uuidstr_copy(char *dest, const char *src) {
    strncpy(dest, src, UUIDSTR_LENGTH);
    dest[UUIDSTR_LENGTH] = '\0';
}

void uuidstr_copy_t(uuidstr_t *dest, const char *src) {
    memcpy(dest->data, src, UUIDSTR_LENGTH);
    dest->zero = 0;
}

bool uuidstr_equals(const char *a, const char *b) {
    return strncasecmp(a, b, UUIDSTR_LENGTH) == 0;
}