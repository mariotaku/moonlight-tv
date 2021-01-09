#pragma once

#include <lgnc_egl.h>

int open_display(int width, int height, EGLNativeWindowType displayId);

void gfx_commit();

int init_egl(EGLNativeWindowType displayId);
void finalize_egl();