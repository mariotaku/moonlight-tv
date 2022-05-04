#include "util/os_info.h"
#include "util/logging.h"

#include <pbnjson.h>

#include "lunasynccall.h"

static bool read_field(char *dest, jvalue_ref body, const char *key);

int parse_release(const char *release);

int os_info_get(os_info_t *info) {
    memset(info, 0, sizeof(os_info_t));
    strncpy(info->name, "webOS", sizeof(info->name));
    char *payload = NULL;
    if (!HLunaServiceCallSync("luna://com.webos.service.tv.systemproperty/getSystemInfo",
                         "{\"keys\":[\"firmwareVersion\", \"sdkVersion\"]}", true, &payload) || !payload) {
        strncpy(info->release, "0.0.0", sizeof(info->release));
        strncpy(info->manufacturing_version, "0.0.0", sizeof(info->manufacturing_version));
        applog_w("OSInfo", "Failed to call luna://com.webos.service.tv.systemproperty/getSystemInfo");
        return -1;
    }
    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
    jdomparser_feed(parser, payload, (int) strlen(payload));
    jdomparser_end(parser);
    jvalue_ref os_info = jdomparser_get_result(parser);
    read_field(info->release, os_info, "sdkVersion");
    read_field(info->manufacturing_version, os_info, "firmwareVersion");
    jdomparser_release(&parser);
    info->version = parse_release(info->release);
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

int parse_release(const char *release) {
    char tmp[32];
    strncpy(tmp, release, 32);
    char *token = NULL;
    int version = 0, factor = 10000;
    while ((token = strtok(token ? NULL : tmp, ".")) != NULL) {
        version += strtol(token, NULL, 10) * factor;
        factor /= 100;
        if (!factor) break;
    }
    return version;
}