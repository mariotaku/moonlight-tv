#include "stream/module/api.h"
#include <stddef.h>

#define base64_encode PLUGIN_SYMBOL_NAME(base64_encode)

unsigned char *base64_encode(const unsigned char *src, size_t len,
                             size_t *out_len);