#include "app_launch.h"

#include <stddef.h>
#include <stdlib.h>

void app_launch_param_free(app_launch_params_t *param) {
    if (param == NULL) {
        return;
    }
    free(param);
}