#pragma once

#include "sys/types.h"

#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API nk_bool nk_imageloadf(const char *path, struct nk_image *img);

NK_API nk_bool nk_imageloadm(const void *mem, size_t size, struct nk_image *img);

NK_API nk_bool nk_image2texture(struct nk_image *img);

NK_API size_t nk_imagebmppxsize(struct nk_image *img);

NK_API void nk_imagebmpfree(struct nk_image *img);

NK_API void nk_imagetexturefree(struct nk_image *img);