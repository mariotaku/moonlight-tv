#include <stdio.h>

#include "ndl_common.h"
#include "util/logging.h"

bool media_loaded = false;

NDL_DIRECTMEDIA_DATA_INFO media_info;

void media_load_callback(int type, long long numValue, const char *strValue)
{
    applog_d("NDL", "MediaLoadCallback type=%d, numValue=%x, strValue=%p", type, numValue, strValue);
}