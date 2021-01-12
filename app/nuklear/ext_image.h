#pragma once
#ifndef NK_NUKLEAR_H_
#include "nuklear.h"
#endif

NK_API nk_bool nk_loadimage(const char *path, struct nk_image *img);

NK_API nk_bool nk_conv2gl(struct nk_image *img);

NK_API void nk_freeimage(struct nk_image *img);