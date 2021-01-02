#include "applications_window.h"
#include "backend/application_manager.h"

void applications_window_init(struct nk_context *ctx)
{
}

bool applications_window(struct nk_context *ctx, const char *selected_address)
{
    PAPP_LIST apps = application_manager_list_of(selected_address);
    int content_height_remaining;
    if (nk_begin(ctx, "Applications", nk_rect(100, 100, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE))
    {
        if (apps != NULL)
        {
            content_height_remaining = (int)nk_window_get_content_region_size(ctx).y;
            content_height_remaining -= ctx->style.window.padding.y * 2;
            struct nk_list_view list_view;
            nk_layout_row_dynamic(ctx, content_height_remaining, 1);
            int app_len = applist_len(apps);
            if (nk_list_view_begin(ctx, &list_view, "apps_list", NK_WINDOW_BORDER, 25, app_len))
            {
                nk_layout_row_dynamic(ctx, 25, 1);
                PAPP_LIST cur = applist_nth(apps, list_view.begin);

                for (int i = 0; i < list_view.count; ++i, cur = cur->next)
                {
                    if (nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
                    {
                    }
                    nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "%d. %s", list_view.begin + i + 1, cur->name);
                }
                nk_list_view_end(&list_view);
            }
        }
    }
    nk_end(ctx);

    // Why Nuklear why, the button looks like "close" but it actually "hide"
    if (nk_window_is_hidden(ctx, "Applications"))
    {
        nk_window_close(ctx, "Applications");
        return false;
    }
    return true;
}