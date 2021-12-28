#include "ini_ext.h"

int ini_write_section(FILE *fp, const char *name) {
    return fprintf(fp, "[%s]\r\n", name);
}

int ini_write_string(FILE *fp, const char *name, const char *value) {
    return fprintf(fp, "%s = %s\r\n", name, value);
}

int ini_write_int(FILE *fp, const char *name, int value) {
    return fprintf(fp, "%s = %d\r\n", name, value);
}

int ini_write_bool(FILE *fp, const char *name, bool value) {
    return fprintf(fp, "%s = %s\r\n", name, value ? "true" : "false");
}