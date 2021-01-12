#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_image.h"

#include <GLES2/gl2.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

NK_API nk_bool nk_loadimage(const char *path, struct nk_image *img)
{
    int x, y, n;
    unsigned char *data = stbi_load(path, &x, &y, &n, 0);
    if (!data)
    {
        return nk_false;
    }
    img->w = x;
    img->h = y;
    img->region[0] = 0;
    img->region[1] = 0;
    img->region[2] = x;
    img->region[3] = y;
    img->handle.ptr = data;
    return nk_true;
}

NK_API nk_bool nk_conv2gl(struct nk_image *img)
{
    unsigned char *data = img->handle.ptr;
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return nk_true;
}

NK_API void nk_freeimage(struct nk_image *img)
{
    GLuint t[1] = {img->handle.id};
    return glDeleteTextures(1, t);
}