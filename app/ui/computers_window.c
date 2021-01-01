#include <stdio.h>

#include <glib.h>

#include "computers_window.h"

void computers_window(struct nk_context *ctx)
{
    /* GUI */
    if (nk_begin(ctx, "Computers", nk_rect(50, 50, 300, 325),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200)))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200)))
        {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);

        struct nk_list_view list_view;
        GList *computer_list = computer_manager_list();
        guint computer_len = g_list_length(computer_list);
        nk_layout_row_dynamic(ctx, 300, 1);
        if (nk_list_view_begin(ctx, &list_view, "computers_list", NK_WINDOW_BORDER, 25, computer_len))
        {
            nk_layout_row_dynamic(ctx, 25, 1);
            GList *cur = g_list_nth(computer_list, list_view.begin);
            for (int i = 0; i < list_view.count; ++i)
            {
                NVCOMPUTER *item = (NVCOMPUTER *)cur->data;
                nk_label(ctx, item->name, NK_TEXT_ALIGN_LEFT);
                cur = g_list_next(cur);
            }
            nk_list_view_end(&list_view);
        }
    }
    nk_end(ctx);
}