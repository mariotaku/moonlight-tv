#pragma once

#include <stdbool.h>
#include <assert.h>

#define UUIDSTR_LENGTH 36
#define UUIDSTR_CAPACITY 37

typedef struct uuidstr_t {
    char data[UUIDSTR_LENGTH];
    char zero;
} uuidstr_t;

static_assert(sizeof(uuidstr_t) == UUIDSTR_CAPACITY, "UUID String is exact 37 bytes");

void uuidstr_fromstr(uuidstr_t *dest, const char *src);

bool uuidstr_strequals(const char *a, const char *b);

bool uuidstr_t_equals_s(const uuidstr_t *a, const char *b);

bool uuidstr_t_equals_t(const uuidstr_t *a, const uuidstr_t *b);