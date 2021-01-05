#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "settings_window.h"
#include "gui_root.h"

#define WINDOW_TITLE "Settings"

void settings_window_init(struct nk_context *ctx)
{
}

bool settings_window(struct nk_context *ctx)
{
    if (nk_begin(ctx, WINDOW_TITLE, nk_rect(100, 50, gui_display_width - 200, gui_display_height - 100),
                 NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_TITLE))
    {
    }
    nk_end(ctx);
    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, WINDOW_TITLE))
    {
        nk_window_close(ctx, WINDOW_TITLE);
        return false;
    }
    return true;
}