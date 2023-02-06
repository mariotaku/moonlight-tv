#include "app.h"
#include "stream/platform.h"
#include "util/logging.h"

static char module_error[1024];

const char *module_geterror() {
    return module_error;
}

