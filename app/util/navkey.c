#include "navkey.h"
#include "ui/config.h"

bool navkey_intercept_repeat(NAVKEY_STATE state, uint32_t timestamp)
{
    static uint32_t first_timestamp = 0, last_timestamp = 0, repeat_duration_accel = 1;
    if (state == NAVKEY_STATE_UP)
    {
        first_timestamp = 0;
        last_timestamp = 0;
        repeat_duration_accel = 1;
        return true;
    }
    else if (state == NAVKEY_STATE_DOWN)
    {
        first_timestamp = timestamp;
    }
    repeat_duration_accel = (timestamp - first_timestamp) / 5000 + 1;
    if (repeat_duration_accel == 0)
        repeat_duration_accel = 1;
    else if (repeat_duration_accel > 4)
        repeat_duration_accel = 4;

    if (timestamp - last_timestamp > KEY_REPEAT_DURATION / repeat_duration_accel)
    {
        last_timestamp = timestamp;
        return false;
    }
    return true;
}