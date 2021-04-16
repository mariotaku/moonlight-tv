#include "platform.h"

#include <string.h>

static MODULE_LIB_DEFINITION _pulse_lib = {"pulse", "pulse"};

bool audio_check_sdl(PAUDIO_INFO ainfo);
extern AUDIO_RENDERER_CALLBACKS audio_callbacks_sdl;

AUDIO_SYMBOLS audio_sdl = {
    true,
    NULL,
    &audio_check_sdl,
    NULL,
    &audio_callbacks_sdl,
};

MODULE_DEFINITION audio_definitions[AUDIO_COUNT] = {
    {"SDL Audio", "sdl", NULL, 0, &audio_sdl},
    {"PulseAudio", "pulse", &_pulse_lib, 1, NULL},
};

AUDIO audio_pref_requested;
AUDIO audio_current;
int audio_current_libidx;
AUDIO_INFO audio_info;

static bool checkinit(AUDIO system, int libidx, int argc, char *argv[]);
static bool audio_try_init(AUDIO audio, int argc, char *argv[]);
static void dlerror_log();

AUDIO audio_init(const char *name, int argc, char *argv[])
{
    AUDIO audio = audio_by_id(name);
    audio_pref_requested = audio;
    if (audio != AUDIO_AUTO)
    {
        // Has preferred value
        if (audio_try_init(audio, argc, argv))
        {
            return audio;
        }
        return AUDIO_SDL;
    }
    for (int i = 0; i < audio_orders_len; i++)
    {
        AUDIO audio = audio_orders[i];
        if (audio_try_init(audio, argc, argv))
        {
            return audio;
        }
    }
    return AUDIO_SDL;
}

static void *module_sym(char *fmt, AUDIO audio, int libidx)
{
    char symbol[128];
    snprintf(symbol, sizeof(symbol), fmt, audio_definitions[audio].dynlibs[libidx].suffix);
    return dlsym(RTLD_DEFAULT, symbol);
}

PAUDIO_RENDERER_CALLBACKS audio_get_callbacks(char *audio_device)
{
    MODULE_DEFINITION pdef = audio_definitions[audio_current];
    if (pdef.symbols.ptr)
        return pdef.symbols.audio->callbacks;
    PAUDIO_RENDERER_CALLBACKS cb = module_sym("audio_callbacks_%s", audio_current, audio_current_libidx);
    if (cb)
        return cb;
    return &audio_callbacks_sdl;
}

static bool audio_init_simple(AUDIO audio, int libidx, int argc, char *argv[])
{
    MODULE_DEFINITION pdef = audio_definitions[audio];
    MODULE_INIT_FN fn;
    if (pdef.symbols.ptr)
    {
        fn = pdef.symbols.audio->init;
    }
    else
    {
        fn = module_sym("audio_init_%s", audio, libidx);
    }
    return !fn || fn(argc, argv);
}

static void audio_finalize_simple(AUDIO audio, int libidx)
{
    MODULE_DEFINITION pdef = audio_definitions[audio];
    MODULE_FINALIZE_FN fn;
    if (pdef.symbols.ptr)
    {
        fn = pdef.symbols.audio->finalize;
    }
    else
    {
        fn = module_sym("audio_finalize_%s", audio, libidx);
    }
    if (fn)
        fn();
}

static bool audio_check_simple(AUDIO audio, int libidx)
{
    memset(&audio_info, 0 , sizeof(audio_info));
    MODULE_DEFINITION pdef = audio_definitions[audio];
    AUDIO_CHECK_FN fn;
    if (pdef.symbols.ptr)
        fn = pdef.symbols.audio->check;
    else
        fn = module_sym("audio_check_%s", audio, libidx);
    if (fn == NULL)
    {
        audio_info.valid = true;
        return true;
    }
    return fn(&audio_info) && audio_info.valid;
}

bool audio_try_init(AUDIO audio, int argc, char *argv[])
{
    char libname[64];
    MODULE_DEFINITION pdef = audio_definitions[audio];
    if (pdef.symbols.ptr && pdef.symbols.audio->valid)
    {
        if (checkinit(audio, -1, argc, argv))
        {
            audio_current = audio;
            audio_current_libidx = -1;
            return true;
        }
    }
    else
    {
        for (int i = 0; i < pdef.liblen; i++)
        {
            snprintf(libname, sizeof(libname), "libmoonlight-%s.so", pdef.dynlibs[i].library);
            // Lazy load to test if this library can be linked
            if (!dlopen(libname, RTLD_NOW | RTLD_GLOBAL))
            {
                dlerror_log();
                continue;
            }
            if (checkinit(audio, i, argc, argv))
            {
                audio_current = audio;
                audio_current_libidx = i;
                return true;
            }
        }
    }
    return false;
}

bool checkinit(AUDIO system, int libidx, int argc, char *argv[])
{
    if (!audio_init_simple(system, libidx, argc, argv))
        return false;
    if (!audio_check_simple(system, libidx))
    {
        audio_finalize_simple(system, libidx);
        return false;
    }
    return true;
}

void audio_finalize()
{
    audio_finalize_simple(audio_current, audio_current_libidx);
}

AUDIO audio_by_id(const char *id)
{
    if (!id || id[0] == 0 || strcmp(id, "auto") == 0)
        return AUDIO_AUTO;
    for (int i = 0; i < AUDIO_COUNT; i++)
    {
        MODULE_DEFINITION pdef = audio_definitions[i];
        if (!pdef.id)
            continue;
        if (strcmp(pdef.id, id) == 0)
            return i;
    }
    return AUDIO_SDL;
}

void dlerror_log()
{
    fprintf(stderr, "Unable to load audio module: %s\n", dlerror());
}
