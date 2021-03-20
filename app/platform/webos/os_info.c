#include "os_info.h"

#include <stdio.h>
#include <pbnjson.h>

int webos_os_info_get_release(char *value, size_t len)
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
    int ret = snprintf(value, len, "%.*s", webos_release_buf.m_len, webos_release_buf.m_str);
    jstring_free_buffer(webos_release_buf);
    return ret;
}