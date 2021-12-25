#include <pbnjson.h>

#include <dlfcn.h>

static int noop() {
    return 0;
}

const char* jvalue_stringify(jvalue_ref val) {
    const char *(*fn)(jvalue_ref) = noop;
    if (fn == noop) {
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