#include "app.h"
#include "logging.h"
#include <webos-helpers/libhelpers.h>
#include <pbnjson.h>

struct app_wakelock_t {
    HContext context;
    __attribute__((unused)) char padding[128];
};

static app_wakelock_t *app_wakelock_create();

static void app_wakelock_destroy(app_wakelock_t *wakelock);

static bool wakelock_respond(LSHandle *sh, LSMessage *reply, void *ctx);

void app_set_keep_awake(app_t *app, bool awake) {
    if (awake) {
        if (app->wakelock != NULL) {
            return;
        }
        app->wakelock = app_wakelock_create();
    } else {
        if (app->wakelock == NULL) {
            return;
        }
        app_wakelock_destroy(app->wakelock);
        app->wakelock = NULL;
    }
}


static app_wakelock_t *app_wakelock_create() {
    char client_name[64];
    snprintf(client_name, 64, "%s.wakelock", getenv("APPID"));
    jvalue_ref payload = jobject_create_var(
            jkeyval(J_CSTR_TO_JVAL("subscribe"), jboolean_create(true)),
            jkeyval(J_CSTR_TO_JVAL("clientName"), j_cstr_to_jval(client_name)),
            NULL
    );
    app_wakelock_t *wakelock = calloc(1, sizeof(app_wakelock_t));
    wakelock->context.multiple = 1;
    wakelock->context.pub = 1;
    wakelock->context.callback = wakelock_respond;

    HLunaServiceCall("luna://com.webos.service.tvpower/power/registerScreenSaverRequest", jvalue_stringify(payload),
                     &wakelock->context);

    j_release(&payload);
    return wakelock;
}

static void app_wakelock_destroy(app_wakelock_t *wakelock) {
    HUnregisterServiceCallback(&wakelock->context);
    free(wakelock);
}

static bool wakelock_respond(LSHandle *sh, LSMessage *reply, void *ctx) {
    (void) sh;
    (void) ctx;
    const char *message = HLunaServiceMessage(reply);
    JSchemaInfo schema_info;
    jschema_info_init(&schema_info, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema_info, 0);

    commons_log_info("Wakelock", "Got ScreenSaverRequest %s", message);
    if (!jdomparser_feed(parser, message, (int) strlen(message))) {
        commons_log_warn("Wakelock", "Failed to feed json: %s", jdomparser_get_error(parser));
        jdomparser_release(&parser);
        return false;
    }
    if (!jdomparser_end(parser)) {
        commons_log_warn("Wakelock", "Failed to finish parsing: %s", jdomparser_get_error(parser));
        jdomparser_release(&parser);
        return false;
    }
    jvalue_ref request = jdomparser_get_result(parser);
    if (!jis_valid(request)) {
        commons_log_warn("Wakelock", "Failed to parse json: %s", jdomparser_get_error(parser));
        jdomparser_release(&parser);
        return false;
    }
    jvalue_ref timestamp = jobject_get(request, J_CSTR_TO_BUF("timestamp"));

    if (jis_null(timestamp)) {
        commons_log_info("Wakelock", "Skip invalid request (no timestamp)");
        jdomparser_release(&parser);
        return true;
    }
    char client_name[64];
    snprintf(client_name, 64, "%s.wakelock", getenv("APPID"));
    jvalue_ref response = jobject_create_var(
            jkeyval(J_CSTR_TO_JVAL("clientName"), j_cstr_to_jval(client_name)),
            jkeyval(J_CSTR_TO_JVAL("ack"), jboolean_create(false)),
            jkeyval(J_CSTR_TO_JVAL("timestamp"), timestamp),
            NULL
    );

    HContext resp_context = {
            .pub = 1,
            .multiple = 0,
    };

    HLunaServiceCall("luna://com.webos.service.tvpower/power/responseScreenSaverRequest", jvalue_stringify(response),
                     &resp_context);
    commons_log_info("Wakelock", "Response ScreenSaverRequest with %s", jvalue_stringify(response));
    j_release(&response);
    jdomparser_release(&parser);
    return true;
}
