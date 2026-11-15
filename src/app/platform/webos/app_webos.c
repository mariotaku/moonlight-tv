#include "app.h"
#include "app_webos.h"
#include "app_launch.h"

#include <pbnjson.h>

#include "logging.h"
#include "util/path.h"
#include "util/i18n.h"

#include "lunasynccall.h"

static char locale_system[16];

void app_open_url(const char *url) {
    SDL_OpenURL(url);
}

void app_init_locale() {
    if (app_configuration->language[0] && strcmp(app_configuration->language, "auto") != 0) {
        commons_log_debug("APP", "Override language to %s", app_configuration->language);
        i18n_setlocale(app_configuration->language);
        return;
    }
    SDL_Locale *locales = SDL_GetPreferredLocales();
    if (locales) {
        for (int i = 0; locales[i].language; i++) {
            if (locales[i].country) {
                snprintf(locale_system, sizeof(locale_system), "%s-%s", locales[i].language, locales[i].country);
            } else {
                strncpy(locale_system, locales[i].language, sizeof(locale_system));
            }
            i18n_setlocale(locale_system);
        }
        SDL_free(locales);
    }
}

app_launch_params_t *app_handle_launch(app_t *app, int argc, char *argv[]) {
    (void) app;
    if (argc < 2) {
        return NULL;
    }
    commons_log_info("APP", "Launched with parameters %s", argv[1]);
    JSchemaInfo schema_info;
    jschema_info_init(&schema_info, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema_info, 0);
    if (!jdomparser_feed(parser, argv[1], (int) strlen(argv[1]))) {
        jdomparser_release(&parser);
        return NULL;
    }
    if (!jdomparser_end(parser)) {
        jdomparser_release(&parser);
        return NULL;
    }
    jvalue_ref params_obj = jdomparser_get_result(parser);
    if (!jis_valid(params_obj)) {
        jdomparser_release(&parser);
        return NULL;
    }
    app_launch_params_t *params = calloc(1, sizeof(app_launch_params_t));
    jvalue_ref host_uuid = jobject_get(params_obj, J_CSTR_TO_BUF("host_uuid"));
    if (jis_string(host_uuid)) {
        raw_buffer buf = jstring_get(host_uuid);
        uuidstr_fromchars(&params->default_host_uuid, buf.m_len, buf.m_str);
    }
    jvalue_ref host_app_id = jobject_get(params_obj, J_CSTR_TO_BUF("host_app_id"));
    if (jis_number(host_app_id)) {
        if (jnumber_get_i32(host_app_id, &params->default_app_id) != CONV_OK) {
            params->default_app_id = 0;
        }
    } else if (jis_string(host_app_id)) {
        raw_buffer buf = jstring_get(host_app_id);
        char *id_str = strndup(buf.m_str, buf.m_len);
        params->default_app_id = strtol(id_str, NULL, 10);
        if (params->default_app_id < 0) {
            params->default_app_id = 0;
        }
        free(id_str);
    }
    jdomparser_release(&parser);
    return params;
}

void app_webos_open_ribbon() {
    static const char *payload = "{\"id\":\"com.webos.app.home\",\"params\":{\"launchType\":\"homeKey\"}}";
    HLunaServiceCallSync("luna://com.webos.applicationManager/launch", payload, true, NULL);
}

/**
 * Call a luna method and copy a string value from its reply.
 * @param object_key Key of the object holding the value, or NULL if it's in the reply root
 */
static bool luna_get_string(const char *uri, const char *payload, const char *object_key, const char *key,
                            char *buf, size_t size) {
    char *reply = NULL;
    if (!HLunaServiceCallSync(uri, payload, true, &reply) || reply == NULL) {
        return false;
    }
    bool found = false;
    JSchemaInfo schema_info;
    jschema_info_init(&schema_info, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema_info, 0);
    if (jdomparser_feed(parser, reply, (int) strlen(reply)) && jdomparser_end(parser)) {
        jvalue_ref value = jdomparser_get_result(parser);
        if (object_key != NULL) {
            value = jobject_get(value, j_cstr_to_buffer(object_key));
        }
        value = jobject_get(value, j_cstr_to_buffer(key));
        if (jis_string(value)) {
            raw_buffer str = jstring_get_fast(value);
            if (str.m_len > 0) {
                size_t len = (size_t) str.m_len < size - 1 ? (size_t) str.m_len : size - 1;
                memcpy(buf, str.m_str, len);
                buf[len] = '\0';
                found = true;
            }
        }
    }
    jdomparser_release(&parser);
    free(reply);
    return found;
}

bool app_load_device_name(char *buf, size_t size) {
    // The TV name in Settings -> General -> Devices. The settings service lets apps on the public bus read this key on
    // newer firmware; on older ones it's refused, and the model name is the next best thing.
    if (luna_get_string("luna://com.webos.settingsservice/getSystemSettings",
                        "{\"category\":\"network\",\"keys\":[\"deviceName\"]}", "settings", "deviceName", buf, size)) {
        return true;
    }
    commons_log_info("APP", "TV name unavailable, using model name for pairing");
    return luna_get_string("luna://com.webos.service.tv.systemproperty/getSystemInfo", "{\"keys\":[\"modelName\"]}",
                           NULL, "modelName", buf, size);
}
