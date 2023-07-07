#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define INI_FULL_MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)
#define INI_NAME_MATCH(n) (strcmp(name, n) == 0)
#define INI_IS_TRUE(v) (strcmp("true", v) == 0)

int ini_write_section(FILE *fp, const char *name);

int ini_write_string(FILE *fp, const char *name, const char *value);

int ini_write_int(FILE *fp, const char *name, int value);

int ini_write_bool(FILE *fp, const char *name, bool value);

int ini_write_comment(FILE *fp, const char *fmt, ...);