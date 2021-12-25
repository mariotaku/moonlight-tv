#include <pbnjson.h>

#include <dlfcn.h>
#include <stdarg.h>

static int noop() {
    return 0;
}

const char *jvalue_stringify(jvalue_ref val) {
    const char *(*fn)(jvalue_ref) = (void *) noop;
    if (fn == (void *) noop) {
        fn = dlsym(RTLD_NEXT, "jvalue_stringify");
        if (!fn) {
            fn = dlsym(RTLD_NEXT, "jvalue_tostring_simple");
        }
    }
    if (!fn) {
        return "{}";
    }
    return fn(val);
}

jvalue_ref jobject_get_nested(jvalue_ref obj, ...) {
    va_list iter;
    va_start(iter, obj);
    const char *key;
    while ((key = va_arg(iter, const char *)) != NULL) {
        if (!jobject_get_exists(obj, j_cstr_to_buffer(key), &obj)) {
            obj = jinvalid();
            break;
        }
    }
    va_end(iter);
    return obj;
}