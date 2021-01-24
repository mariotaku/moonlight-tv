#include "graphics.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <GLES2/gl2.h>

EGLDisplay g_pstEglDisplay = NULL;
EGLConfig g_pstEglConfig = NULL;
EGLSurface g_pstEglSurface = NULL;
EGLContext g_pstEglContext = NULL;

int open_display(int width, int height, EGLNativeWindowType displayId)
{
    if (init_egl(displayId) == 0)
    {
        finalize_egl();
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 0);
    glClearDepthf(1.0f);
    glColorMask(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 0;
}

int init_egl(EGLNativeWindowType displayId)
{
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
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_NONE};

    static const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE};

    g_pstEglDisplay = LGNC_eglGetDisplay();
    if (g_pstEglDisplay == EGL_NO_DISPLAY)
    {
        fprintf(stderr, "ERROR, cannot create create egl g_pstDisplay\n");
        return 0;
    }

    if (LGNC_eglInitialize(g_pstEglDisplay, &major, &minor) != EGL_TRUE)
    {
        fprintf(stderr, "ERROR, cannot initialize egl g_pstDisplay\n");
        return 0;
    }

    if (LGNC_eglGetConfigs(g_pstEglDisplay, NULL, 0, &count) != EGL_TRUE)
    {
        fprintf(stderr, "ERROR, eglGetConfigs failed\n");
        return 0;
    }
    configs = (EGLConfig *)calloc(count, sizeof(EGLConfig));
    LGNC_eglChooseConfig(g_pstEglDisplay, configAttributes, configs, count, &n);
    // simply choose the first config
    g_pstEglConfig = configs[0];

    g_pstEglSurface = LGNC_eglCreateWindowSurface(g_pstEglDisplay, g_pstEglConfig, displayId, NULL);
    if (g_pstEglSurface == EGL_NO_SURFACE)
    {
        fprintf(stderr, "ERROR, cannot create create egl g_pstEglSurface\n");
        return 0;
    }

    g_pstEglContext = LGNC_eglCreateContext(g_pstEglDisplay, g_pstEglConfig, EGL_NO_CONTEXT, contextAttributes);
    if (g_pstEglContext == EGL_NO_CONTEXT)
    {
        fprintf(stderr, "ERROR, cannot create create egl g_pstEglContext\n");
        return 0;
    }

    if (LGNC_eglMakeCurrent(g_pstEglDisplay, g_pstEglSurface, g_pstEglSurface, g_pstEglContext) != EGL_TRUE)
    {
        fprintf(stderr, "ERROR, cannot make current\n");
        return 0;
    }
    LGNC_eglSwapInterval(g_pstEglDisplay, 1);
    return 1;
}

void finalize_egl()
{
    if (g_pstEglDisplay == NULL)
    {
        return;
    }
    LGNC_eglMakeCurrent(g_pstEglDisplay, 0, 0, 0);
    LGNC_eglDestroySurface(g_pstEglDisplay, g_pstEglSurface);
    LGNC_eglDestroyContext(g_pstEglDisplay, g_pstEglContext);
    LGNC_eglTerminate(g_pstEglDisplay);
}

void gfx_commit()
{
    LGNC_eglSwapBuffers(g_pstEglDisplay, g_pstEglSurface);
}