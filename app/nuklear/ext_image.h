#pragma once

#include "sys/types.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API nk_bool nk_loadimgfile(const char *path, struct nk_image *img);

NK_API nk_bool nk_loadimgmem(const void *mem, size_t size, struct nk_image *img);

NK_API nk_bool nk_conv2gl(struct nk_image *img);

NK_API void nk_freeimage(struct nk_image *img);