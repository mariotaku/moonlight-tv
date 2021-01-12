#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_image.h"

#include <SDL_image.h>
#include <SDL_opengl.h>

static GLuint gen_texture_from_sdl(SDL_Surface *surface);

NK_API nk_bool nk_loadimage(const char *path, struct nk_image *img)
{
    SDL_Surface *s = IMG_Load(path);
    if (!s)
    {
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

NK_API nk_bool nk_conv2gl(struct nk_image *img)
{
    SDL_Surface *surface = img->handle.ptr;
    int texture = gen_texture_from_sdl(surface);
    img->handle.id = texture;
    // Surface has been used up
    SDL_FreeSurface(surface);
}

NK_API void nk_freeimage(struct nk_image *img)
{
    GLuint t[1] = {img->handle.id};
    return glDeleteTextures(1, t);
}

GLuint gen_texture_from_sdl(SDL_Surface *surface)
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
        printf("warning: the image is not truecolor..  this will probably break\n");
        // this error should not go unhandled
    }

    // Have OpenGL generate a texture object handle for us
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, texture_format, surface->w, surface->h, 0,
                 texture_format, GL_UNSIGNED_BYTE, surface->pixels);
    return texture;
}