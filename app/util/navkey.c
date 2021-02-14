#include "navkey.h"
#include "ui/config.h"

bool navkey_intercept_repeat(bool down, uint32_t timestamp)
{
    static uint32_t last_timestamp;
    if (!down)
    {
        last_timestamp = 0;
        return true;
    }
    if (timestamp - last_timestamp > KEY_REPEAT_DURATION)
    {
        last_timestamp = timestamp;
        return false;
    }
    return true;
}