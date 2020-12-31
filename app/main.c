/* nuklear - 1.40.8 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include <glib.h>
#include <wayland-webos-shell-client-protocol.h>
#include <NDL_directmedia.h>

#define NK_IMPLEMENTATION
#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/nuklear_wayland_egl.h"

#include "main.h"
#include "gst_sample.h"
#include "debughelper.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define UNUSED(a) (void)a
#define LEN(a) (sizeof(a) / sizeof(a)[0])

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the include
 * and the corresponding function. */
/*#include "../style.c"*/
#include "ui/application_root.h"

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/

struct wl_shell *g_pstShell = NULL;
struct wl_webos_shell *g_pstWebOSShell = NULL;

static const char APPID[] = "com.limelight.webos";

static int exit_requested_;

static void finalize();

//WAYLAND POINTER INTERFACE (mouse/touchpad)
static void nk_wayland_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

static void nk_wayland_pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
}

static void nk_wayland_pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;
    win->mouse_pointer_x = wl_fixed_to_int(x);
    win->mouse_pointer_y = wl_fixed_to_int(y);

    // fprintf(stderr, "pointer motion: %d,%d \n", win->mouse_pointer_x, win->mouse_pointer_y);

    nk_input_motion(&(win->ctx), win->mouse_pointer_x, win->mouse_pointer_y);
}

static void nk_wayland_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;
    fprintf(stderr, "pointer button: %d, pressed: %d \n", button, state);

    if (button == 272)
    { //left mouse button
        if (state == WL_POINTER_BUTTON_STATE_PRESSED)
        {
            // printf("nk_input_button x=%d, y=%d press: 1 \n", win->mouse_pointer_x, win->mouse_pointer_y);
            nk_input_button(&(win->ctx), NK_BUTTON_LEFT, win->mouse_pointer_x, win->mouse_pointer_y, 1);
        }
        else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
        {
            nk_input_button(&(win->ctx), NK_BUTTON_LEFT, win->mouse_pointer_x, win->mouse_pointer_y, 0);
        }
    }
}

static void nk_wayland_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static struct wl_pointer_listener nk_wayland_pointer_listener =
    {
        &nk_wayland_pointer_enter,
        &nk_wayland_pointer_leave,
        &nk_wayland_pointer_motion,
        &nk_wayland_pointer_button,
        &nk_wayland_pointer_axis};
//-------------------------------------------------------------------- endof WAYLAND POINTER INTERFACE

//WAYLAND KEYBOARD INTERFACE
static void nk_wayland_keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size)
{
}

static void nk_wayland_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
}

static void nk_wayland_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
}

static void nk_wayland_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    fprintf(stderr, "key: %d \n", key);
}

static void nk_wayland_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static struct wl_keyboard_listener nk_wayland_keyboard_listener =
    {
        &nk_wayland_keyboard_keymap,
        &nk_wayland_keyboard_enter,
        &nk_wayland_keyboard_leave,
        &nk_wayland_keyboard_key,
        &nk_wayland_keyboard_modifiers};
//-------------------------------------------------------------------- endof WAYLAND KEYBOARD INTERFACE

//WAYLAND SEAT INTERFACE
static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        struct wl_pointer *pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &nk_wayland_pointer_listener, win);
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(keyboard, &nk_wayland_keyboard_listener, win);
    }
}

static struct wl_seat_listener seat_listener =
    {
        &seat_capabilities};
//-------------------------------------------------------------------- endof WAYLAND SEAT INTERFACE

static void registryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    struct nk_wl_egl *win = (struct nk_wl_egl *)data;
    if (strcmp(interface, "wl_compositor") == 0)
    {
        win->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        g_pstShell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
    }
    else if (strcmp(interface, "wl_webos_shell") == 0)
    {
        g_pstWebOSShell = wl_registry_bind(registry, id, &wl_webos_shell_interface, 1);
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        win->seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(win->seat, &seat_listener, win);
    }
}

static void registryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
}

static const struct wl_registry_listener s_stRegistryListener = {
    registryHandler,
    registryRemover};

static void webosShellHandleState(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, uint32_t state)
{
    switch (state)
    {
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
        break;
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
        break;
    }
}

static void webosShellHandlePosition(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, int32_t x, int32_t y)
{
}

static void webosShellHandleClose(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface)
{
}

static void webosShellHandleExpose(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, struct wl_array *rectangles)
{
}

static void webosShellHandleStateAboutToChange(void *data, struct wl_webos_shell_surface *wl_webos_shell_surface, uint32_t state)
{
}

static const struct wl_webos_shell_surface_listener s_pstWebosShellListener = {
    webosShellHandleState,
    webosShellHandlePosition,
    webosShellHandleClose,
    webosShellHandleExpose,
    webosShellHandleStateAboutToChange};

static void getWaylandServer()
{
    struct wl_registry *pstRegistry = NULL;
    struct nk_wl_egl *win = &wl_egl;

    win->display = wl_display_connect(NULL);
    if (win->display == NULL)
    {
        fprintf(stderr, "ERROR, cannot connect!\n");
        exit(1);
    }

    pstRegistry = wl_display_get_registry(win->display);
    wl_registry_add_listener(pstRegistry, &s_stRegistryListener, &wl_egl);

    wl_display_dispatch(win->display);
    // wait for a synchronous response
    wl_display_roundtrip(win->display);

    if ((&wl_egl)->compositor == NULL || g_pstShell == NULL || g_pstWebOSShell == NULL)
    {
        fprintf(stderr, "ERROR, cannot find compositor / shell\n");
        exit(1);
    }

    (&wl_egl)->surface = wl_compositor_create_surface((&wl_egl)->compositor);
    if ((&wl_egl)->surface == NULL)
    {
        fprintf(stderr, "ERROR, cannot create surface \n");
        exit(1);
    }

    struct wl_shell_surface *shell_surface = wl_shell_get_shell_surface(g_pstShell, (&wl_egl)->surface);
    if (shell_surface == NULL)
    {
        fprintf(stderr, "Can't create shell surface\n");
        exit(1);
    }
    wl_shell_surface_set_toplevel(shell_surface);

    struct wl_webos_shell_surface *webos_shell_surface = wl_webos_shell_get_shell_surface(g_pstWebOSShell, (&wl_egl)->surface);
    if (webos_shell_surface == NULL)
    {
        fprintf(stderr, "Can't create webos shell surface\n");
        exit(1);
    }
    wl_webos_shell_surface_add_listener(webos_shell_surface, &s_pstWebosShellListener, win->display);
    wl_webos_shell_surface_set_property(webos_shell_surface, "appId", (getenv("APPID") ? getenv("APPID") : APPID));
    // for secondary display, set the last parameter as 1
    wl_webos_shell_surface_set_property(webos_shell_surface, "displayAffinity", (getenv("DISPLAY_ID") ? getenv("DISPLAY_ID") : "0"));
    wl_webos_shell_surface_set_property(webos_shell_surface, "_WEBOS_ACCESS_POLICY_KEYS_BACK", "true");
}

static void createWindow()
{
    struct nk_wl_egl *win = &wl_egl;
    win->width = WINDOW_WIDTH;
    win->height = WINDOW_HEIGHT;

    // webOS only supports full screen size
    win->win = wl_egl_window_create(win->surface, win->width, win->height);

    if (win->win == EGL_NO_SURFACE)
    {
        fprintf(stderr, "ERROR, cannot create wayland egl window\n");
        exit(1);
    }

    win->egl_surface = eglCreateWindowSurface(win->egl_display, win->egl_config, win->win, NULL);

    if (!eglMakeCurrent(win->egl_display, win->egl_surface, win->egl_surface, win->egl_context))
    {
        fprintf(stderr, "ERROR, cannot make current\n");
    }

    eglSwapInterval(win->egl_display, 1);
}

static void initEgl()
{
    struct nk_wl_egl *win = &wl_egl;
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    int i;

    EGLint configAttributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0x18,
        EGL_STENCIL_SIZE, 0,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_NONE};

    static const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE};

    win->egl_display = eglGetDisplay((EGLNativeDisplayType)win->display);
    if (win->egl_display == EGL_NO_DISPLAY)
    {
        fprintf(stderr, "ERROR, cannot create create egl g_pstDisplay\n");
        exit(1);
    }

    if (eglInitialize(win->egl_display, &major, &minor) != EGL_TRUE)
    {
        fprintf(stderr, "ERROR, cannot initialize egl g_pstDisplay\n");
        exit(1);
    }
    eglGetConfigs(win->egl_display, NULL, 0, &count);
    configs = (EGLConfig *)calloc(count, sizeof(EGLConfig));
    eglChooseConfig(win->egl_display, configAttributes, configs, count, &n);
    // simply choose the first config
    win->egl_config = configs[0];
    win->egl_context = eglCreateContext(win->egl_display, win->egl_config, EGL_NO_CONTEXT, contextAttributes);
}

static void finalize()
{
    struct nk_wl_egl *win = &wl_egl;
    eglDestroyContext(win->egl_display, win->egl_context);
    eglDestroySurface(win->egl_display, win->egl_surface);
    eglTerminate(win->egl_display);
    wl_display_disconnect(win->display);
}

static gboolean
nk_main_loop(gpointer user_data)
{
    struct nk_context *ctx = user_data;
    struct nk_wl_egl *win = &wl_egl;

    //handle wayland stuff (send display to FB & get inputs)
    nk_input_begin(ctx);
    wl_display_dispatch(win->display);
    nk_input_end(ctx);

    application_root(ctx);

    application_background();

    /* IMPORTANT: `nk_wl_egl_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    nk_wl_egl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    eglSwapBuffers(win->egl_display, win->egl_surface);
    return TRUE;
}

static struct nk_context *gui_init_nk()
{
    struct nk_context *ctx;
    struct nk_wl_egl *win = &wl_egl;
    getWaylandServer();
    initEgl();
    createWindow();

    /* OpenGL setup */
    glViewport(0, 0, win->width, win->height);

    ctx = nk_wl_egl_init(win->win);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        nk_wl_egl_font_stash_begin(&atlas);
        /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
        /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
        /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
        /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
        /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
        /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
        nk_wl_egl_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        /*nk_style_set_font(ctx, &roboto->handle)*/;
    }

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/
    return ctx;
}

gboolean g_uisrc_prepare(GSource *source, gint *timeout_)
{
    *timeout_ = -1;
    return TRUE;
}

gboolean g_uisrc_check(GSource *source)
{
    return TRUE;
}

gboolean g_uisrc_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
    if (callback(user_data))
    {
        return G_SOURCE_CONTINUE;
    }
    return G_SOURCE_REMOVE;
}

void g_uisrc_finalize(GSource *source)
{
    finalize();
}

int main(int argc, char *argv[])
{
    REDIR_STDOUT("moonlight");

    gst_init(&argc, &argv);

    if (NDL_DirectMediaInit(APPID, NULL))
    {
        g_error(NDL_DirectMediaGetError(), NULL);
        return -1;
    }

    /* GUI */

    struct nk_context *nk_ctx = NULL;
    nk_ctx = gui_init_nk();

    GMainContext *gctx;
    GMainLoop *loop;

    GSourceFuncs uisrc_funcs = {g_uisrc_prepare, g_uisrc_check, g_uisrc_dispatch, g_uisrc_finalize};
    GSource *uisrc;
    uisrc = g_source_new(&uisrc_funcs, sizeof(GSource));

    gctx = g_main_context_new();

    g_source_attach(uisrc, gctx);

    loop = g_main_loop_new(gctx, FALSE);

    g_source_set_callback(uisrc, nk_main_loop, nk_ctx, NULL);

    g_main_loop_run(loop);

    nk_wl_egl_shutdown();

    g_main_loop_unref(loop);

    return 0;
}

void request_exit()
{
    exit_requested_ = 1;
}

int exit_requested()
{
    return exit_requested_;
}