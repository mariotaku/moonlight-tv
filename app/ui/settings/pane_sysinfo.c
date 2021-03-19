#include "window.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if OS_WEBOS
#include <pbnjson.h>
#endif

#if OS_WEBOS
static char webos_release[32];
#endif

static void load_webos_info();

static bool _render(struct nk_context *ctx, bool *showing_combo)
{
    static struct nk_rect item_bounds = {0, 0, 0, 0};
    int item_index = 0;

    nk_layout_row_template_begin_s(ctx, 25);
    nk_layout_row_template_push_static_s(ctx, 100);
    nk_layout_row_template_push_variable_s(ctx, 1);
    nk_layout_row_template_end(ctx);
#if OS_WEBOS
    nk_label(ctx, "webOS version", NK_TEXT_LEFT);
    nk_label(ctx, webos_release, NK_TEXT_RIGHT);
#endif
    return true;
}

static int _itemcount()
{
    int count = 0;
#if HAS_WEBOS_SETTINGS
    count += 1;
#endif
    return count;
}

static void _onselect()
{
#if OS_WEBOS
    load_webos_info();
#endif
}

#if OS_WEBOS
void load_webos_info()
{
    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
    FILE *f = fopen("/var/run/nyx/os_info.json", "r");
    char buf[8192];
    if (f)
    {
        size_t buflen;
        while ((buflen = fread(buf, sizeof(char), 8192, f)) > 0)
        {
            jdomparser_feed(parser, buf, buflen);
        }
        fclose(f);
    }
    jdomparser_end(parser);
    jvalue_ref os_info = jdomparser_get_result(parser);
    raw_buffer webos_release_buf = jstring_get(jobject_get(os_info, J_CSTR_TO_BUF("webos_release")));
    snprintf(webos_release, sizeof(webos_release), "%.*s", webos_release_buf.m_len, webos_release_buf.m_str);
    jstring_free_buffer(webos_release_buf);
}
#endif
struct settings_pane settings_pane_sysinfo = {
    .title = "System Info",
    .render = _render,
    .navkey = NULL,
    .itemcount = _itemcount,
    .onselect = _onselect,
};
