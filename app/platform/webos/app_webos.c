#include "app.h"

#include <pbnjson.h>

#include "util/logging.h"
#include "util/path.h"
#include "util/i18n.h"

#include "lunasynccall.h"

static char locale_system[16];

void app_open_url(const char *url) {
    jvalue_ref payload_obj = jobject_create_var(
            jkeyval(J_CSTR_TO_JVAL("id"), J_CSTR_TO_JVAL("com.webos.app.browser")),
            jkeyval(J_CSTR_TO_JVAL("params"), jobject_create_var(
                    jkeyval(J_CSTR_TO_JVAL("target"), j_cstr_to_jval(url)),
                    J_END_OBJ_DECL
            )),
            J_END_OBJ_DECL
    );
    const char *payload = jvalue_stringify(payload_obj);
    HLunaServiceCallSync("luna://com.webos.applicationManager/launch", payload, true, NULL);
    j_release(&payload_obj);
}

void app_init_locale() {
    if (app_configuration->language[0] && strcmp(app_configuration->language, "auto") != 0) {
        i18n_setlocale(app_configuration->language);
        return;
    }
    char *payload = NULL;
    if (!HLunaServiceCallSync("luna://com.webos.settingsservice/getSystemSettings", "{\"key\": \"localeInfo\"}",
                              true, &payload) || !payload) {
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