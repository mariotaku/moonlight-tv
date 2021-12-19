#include "stream/input/sdlinput.h"
#include "ui/root.h"

#include <Limelight.h>
#include <libhelpers.h>
#include <pbnjson.h>

#include "util/logging.h"

static bool mrcu_callback(LSHandle *sh, LSMessage *reply, void *ctx);

static HContext mrcu_ctx = {.callback = mrcu_callback, .multiple = 1, .ret_token = 0};
static jdomparser_ref parser = NULL;

bool webos_magic_remote_active() {
    return mrcu_ctx.ret_token != 0;
}

void webos_magic_remote_register() {
    if (mrcu_ctx.ret_token) return;
    parser = jdomparser_new(jschema_all());
    const static char *uri = "luna://com.webos.service.mrcu/sensor/getSensorData";
    const static char *payload = "{\"callbackInterval\": 1, \"subscribe\": true}";
    if (HLunaServiceCall(uri, payload, &mrcu_ctx) != 0) {
        jdomparser_release(&parser);
        applog_w("MRCU", "Failed to call luna://com.webos.service.mrcu/sensor/getSensorData");
        return;
    }
}

void webos_magic_remote_unregister() {
    if (!mrcu_ctx.ret_token) return;
    HUnregisterServiceCallback(&mrcu_ctx);
    mrcu_ctx.ret_token = 0;
    jdomparser_release(&parser);
}

static bool mrcu_callback(LSHandle *sh, LSMessage *reply, void *ctx) {
    if (!absinput_should_accept())
        return true;
    const char *payload = HLunaServiceMessage(reply);
    applog_i("MRCU", payload);
    jdomparser_feed(parser, payload, (int) strlen(payload));
    jdomparser_end(parser);
    jvalue_ref payload_obj = jdomparser_get_result(parser);
    jvalue_ref coordinate_ref = jobject_get(payload_obj, J_CSTR_TO_BUF("coordinate"));
    if (!jis_valid(coordinate_ref)) goto cleanup;
    jvalue_ref x_ref = jobject_get(coordinate_ref, J_CSTR_TO_BUF("x"));
    jvalue_ref y_ref = jobject_get(coordinate_ref, J_CSTR_TO_BUF("y"));
    if (jis_number(x_ref) && jis_number(y_ref)) {
        int x = -1, y = -1;
        jnumber_get_i32(x_ref, &x);
        jnumber_get_i32(y_ref, &y);
        LiSendMousePositionEvent(x, y, ui_display_width, ui_display_height);
    }
    cleanup:
    j_release(&payload_obj);
    return true;
}