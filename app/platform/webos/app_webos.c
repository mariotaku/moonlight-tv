#include "app.h"

#include <pbnjson.h>

#include "util/logging.h"
#include "util/path.h"
#include "util/i18n.h"

#include "lunasynccall.h"

static char locale_system[16];

void app_open_url(const char *url) {
    char payload[8192];
    snprintf(payload, sizeof(payload), "{\"id\": \"com.webos.app.browser\", \"params\":{\"target\": \"%s\"}}", url);
    HLunaServiceCallSync("luna://com.webos.applicationManager/launch", payload, true, NULL);
}

void app_init_locale() {
    if (app_configuration->language[0] && strcmp(app_configuration->language, "auto") != 0) {
        i18n_setlocale(app_configuration->language);
        return;
    }
    char *payload = NULL;
    if (!HLunaServiceCallSync("luna://com.webos.settingsservice/getSystemSettings", "{\"key\": \"localeInfo\"}", true,
                              &payload) || !payload) {
        return;
    }
    JSchemaInfo schemaInfo;
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
    jdomparser_feed(parser, payload, (int) strlen(payload));
    jdomparser_end(parser);
    jvalue_ref payload_obj = jdomparser_get_result(parser);
    jvalue_ref locale = jobject_get_nested(payload_obj, "settings", "localeInfo", "locales", "UI", NULL);
    if (jis_string(locale)) {
        raw_buffer buf = jstring_get(locale);
        size_t len = buf.m_len <= 15 ? buf.m_len : 15;
        strncpy(locale_system, buf.m_str, len);
        locale_system[len] = '\0';
        jstring_free_buffer(buf);
        i18n_setlocale(locale_system);
    }
    jdomparser_release(&parser);
}