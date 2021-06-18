#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_image.h"

#include <SDL_image.h>
#include <SDL_opengl.h>

#include "util/logging.h"

static GLuint gen_texture_from_sdl(SDL_Surface *surface, int subsample);

NK_API nk_bool nk_imageloadf(const char *path, struct nk_image *img)
{
    SDL_Surface *s = IMG_Load(path);
    if (!s)
    {
        applog_w("IMG", "Failed to load image %s: %s", path, IMG_GetError());
        return nk_false;
    }
    img->w = s->w;
    img->h = s->h;
    img->region[0] = 0;
    img->region[1] = 0;
    img->region[2] = s->w;
    img->region[3] = s->h;
    img->handle.ptr = s;
    return nk_true;
}

NK_API nk_bool nk_imageloadm(const void *mem, size_t size, struct nk_image *img)
{
    SDL_RWops *rw = SDL_RWFromConstMem(mem, size);
    SDL_Surface *s = IMG_Load_RW(rw, SDL_TRUE);
    if (!s)
    {
        applog_w("IMG", "Failed to load image %s: %s", path, IMG_GetError());
        return nk_false;
    }
    img->w = s->w;
    img->h = s->h;
    img->region[0] = 0;
    img->region[1] = 0;
    img->region[2] = s->w;
    img->region[3] = s->h;
    img->handle.ptr = s;
    return nk_true;
}

NK_API nk_bool nk_image2texture(struct nk_image *img, int size_limit)
{
    SDL_Surface *surface = img->handle.ptr;
    int subsample = 1;
    int max_size = NK_MAX(img->w, img->h);
    while (size_limit && max_size > size_limit)
    {
        max_size /= 2;
        subsample *= 2;
    }
    if (subsample > 1)
    {
        img->w /= subsample;
        img->h /= subsample;
        img->region[0] /= subsample;
        img->region[1] /= subsample;
        img->region[2] /= subsample;
        img->region[3] /= subsample;
    }
    int texture = gen_texture_from_sdl(surface, subsample);
    // Surface has been used up
    nk_imagebmpfree(img);
    img->handle.id = texture;
    return nk_true;
}

NK_API size_t nk_imagebmppxsize(struct nk_image *img)
{
    SDL_Surface *surface = img->handle.ptr;
    return surface->format->BytesPerPixel;
}

NK_API void nk_imagebmpfree(struct nk_image *img)
{
    SDL_Surface *surface = img->handle.ptr;
    SDL_FreeSurface(surface);
}

NK_API void nk_imagetexturefree(struct nk_image *img)
{
    GLuint t[1] = {img->handle.id};
    return glDeleteTextures(1, t);
}

GLuint gen_texture_from_sdl(SDL_Surface *surface, int subsample)
{
    GLuint texture;
    GLenum texture_format;
    GLint nOfColors;
    // get the number of channels in the SDL surface
    nOfColors = surface->format->BytesPerPixel;
    if (nOfColors == 4) // contains an alpha channel
    {
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGBA;
        else
            texture_format = GL_BGRA;
    }
    else if (nOfColors == 3) // no alpha channel
    {
        if (surface->format->Rmask == 0x000000ff)
            texture_format = GL_RGB;
        else
            texture_format = GL_BGR;
    }
    else
    {
        // this error should not go unhandled
        return 0;
    }

    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (subsample > 1)
    {
        /* Scaled width and height */
        int sw = surface->w / subsample,
            sh = surface->h / subsample;
        SDL_Surface *scaled = SDL_CreateRGBSurface(0, sw, sh, surface->format->BitsPerPixel, surface->format->Rmask,
                                                   surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
        SDL_BlitScaled(surface, NULL, scaled, NULL);
        // Edit the texture object's image data using the information SDL_Surface gives us
        glTexImage2D(GL_TEXTURE_2D, 0, texture_format, scaled->w, scaled->h, 0, texture_format,
                     GL_UNSIGNED_BYTE, scaled->pixels);
        SDL_FreeSurface(scaled);
    }
    else
    {
        // Edit the texture object's image data using the information SDL_Surface gives us
        glTexImage2D(GL_TEXTURE_2D, 0, texture_format, surface->w, surface->h, 0,
                     texture_format, GL_UNSIGNED_BYTE, surface->pixels);
    }
    return texture;
}
