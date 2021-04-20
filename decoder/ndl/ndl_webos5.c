#include "ndl_common.h"

#include <stdio.h>

bool media_loaded = false;

NDL_DIRECTMEDIA_DATA_INFO media_info;

void media_load_callback(int type, long long numValue, const char *strValue)
{
    printf("[NDL] MediaLoadCallback type=%d, numValue=%x, strValue=%p\n");
}