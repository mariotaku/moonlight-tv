#include "os_info.h"

#include <stdio.h>
#include <pbnjson.h>

static bool read_field(char *dest, jvalue_ref body, const char *key);

int webos_os_info_get_release(webos_os_info_t *info) {
    memset(info, 0, sizeof(webos_os_info_t));
    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
    FILE *f = fopen("/var/run/nyx/os_info.json", "r");
    char buf[8192];
    if (f) {
        size_t buflen;
        while ((buflen = fread(buf, sizeof(char), 8192, f)) > 0) {
            jdomparser_feed(parser, buf, (int) buflen);
        }
        fclose(f);
    }
    jdomparser_end(parser);
    jvalue_ref os_info = jdomparser_get_result(parser);
    read_field(info->release, os_info, "webos_release");
    read_field(info->manufacturing_version, os_info, "webos_manufacturing_version");
    return 0;
}

static bool read_field(char *dest, jvalue_ref body, const char *key) {
    jvalue_ref value = jobject_get(body, j_cstr_to_buffer(key));
    if (jis_null(value)) return false;
    raw_buffer webos_release_buf = jstring_get(value);
    memcpy(dest, webos_release_buf.m_str, webos_release_buf.m_len);
    dest[webos_release_buf.m_len] = 0;
    jstring_free_buffer(webos_release_buf);
    return true;
}