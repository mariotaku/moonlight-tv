#include "res.h"

#include <stddef.h>

#include "sdl_renderer.h"
#include "gui_root.h"

#define GL_GLEXT_PROTOTYPES

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <assert.h>

static const char *shader_sources[2] = {res_vertex_source_data, res_fragment_source_data};
static const char *texture_mappings[] = {"ymap", "umap", "vmap"};

static int width, height;

static GLuint texture_id[3], texture_uniform[3];
static GLuint vertex_shader, fragment_shader, shader_program;

void renderer_setup(int w, int h)
{
    width = w;
    height = h;

    SDL_Log("OpenGL version: %s\n", glGetString(GL_VERSION));

    int status;
    char msg[512];
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &shader_sources[0], NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    glGetShaderInfoLog(vertex_shader, sizeof msg, NULL, msg);
    SDL_Log("vertex shader info: %s\n", msg);
    assert(status == GL_TRUE);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &shader_sources[1], NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    glGetShaderInfoLog(vertex_shader, sizeof msg, NULL, msg);
    SDL_Log("fragment shader info: %s\n", msg);
    assert(status == GL_TRUE);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);

    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    glBindAttribLocation(shader_program, 0, "position");
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

    glGenTextures(3, texture_id);
    for (int i = 0; i < 3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, texture_id[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, i > 0 ? width / 2 : width, i > 0 ? height / 2 : height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

        texture_uniform[i] = glGetUniformLocation(shader_program, texture_mappings[i]);
        assert(glGetError() == GL_NO_ERROR);
    }
}

void renderer_submit_frame(void *data1, void *data2)
{
    uint8_t **image = data1;
}

void renderer_draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);

    glColor3f(1.0f, 0.0f, 0.0f);
    glRectf(-0.75f, 0.75f, 0.75f, -0.75f);
}

void renderer_cleanup()
{
    glDeleteProgram(shader_program);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);
}