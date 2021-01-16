#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_image.h"

#include <GLES2/gl2.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct stb_image
{
    int x, y, n;
    unsigned char *data;
};

NK_API nk_bool nk_loadimage(const char *path, struct nk_image *img)
{
    int x, y, n;
    struct stb_image *s = malloc(sizeof(struct stb_image));
    memset(s, 0, sizeof(struct stb_image));
    s->data = stbi_load(path, &x, &y, &n, 0);
    if (!s->data)
    {
        free(s);
        return nk_false;
    }
    s->x = x;
    s->y = y;
    s->n = n;

    img->w = x;
    img->h = y;
    img->region[0] = 0;
    img->region[1] = 0;
    img->region[2] = x;
    img->region[3] = y;
    img->handle.ptr = s;
    return nk_true;
}

NK_API nk_bool nk_image2texture(struct nk_image *img)
{
    struct stb_image *s = img->handle.ptr;
    GLuint tex;
    GLenum texfmt;

    if (s->n == 4)
    {
        texfmt = GL_RGBA;
    }
    else if (s->n == 3)
    {
        texfmt = GL_RGB;
    }
    else
    {
        stbi_image_free(s->data);
        free(s);
        return nk_false;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, texfmt, img->w, img->h, 0, texfmt, GL_UNSIGNED_BYTE, s->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    nk_imagebmpfree(img);
    img->handle.id = tex;

    return nk_true;
}

NK_API void nk_imagebmpfree(struct nk_image *img)
{
    struct stb_image *s = img->handle.ptr;
    stbi_image_free(s->data);
    free(s);
}

NK_API void nk_imagetexturefree(struct nk_image *img)
{
    GLuint t[1] = {img->handle.id};
    return glDeleteTextures(1, t);
}