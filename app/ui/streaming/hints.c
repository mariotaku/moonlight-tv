#include "hints.h"
#include "util/i18n.h"

#include <SDL.h>

#define HINTS_LEN 2
static const char *hints_list[HINTS_LEN] = {
        "Long press BACK (or press EXIT for traditional TV remote) while streaming, to open status overlay.",
        "Frequent lagging on 5GHz Wi-Fi? Try changing to another channel, check Help for details.",
};

const char *hints_obtain() {
    static int hint_idx = -1;
    if (hint_idx < 0) {
        hint_idx = (int) SDL_GetTicks() % HINTS_LEN;
    } else if (++hint_idx >= HINTS_LEN) {
        hint_idx = 0;
    }

    return locstr(hints_list[hint_idx]);
}