#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include <Limelight.h>
#include <pthread.h>
#include "sdl_renderer.h"
#include "sdlvid.h"
#include "stream/api.h"

#include <libavcodec/avcodec.h>

#if TARGET_DESKTOP
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#else
#include <GLES2/gl2.h>
#endif

#define INCBIN_PREFIX res_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include <incbin.h>

#if HAVE_GL2
INCBIN(vertex_source, SOURCE_DIR "/shaders/vertex_source_gl2.glsl");
INCBIN(fragment_source, SOURCE_DIR "/shaders/fragment_source_gl2.glsl");
#elif HAVE_GLES2
INCBIN(vertex_source, SOURCE_DIR "/shaders/vertex_source_gles2.glsl");
INCBIN(fragment_source, SOURCE_DIR "/shaders/fragment_source_gles2.glsl");
#else
#error "No GL version found"
#endif

static const float vertices[] = {
    -1.f, 1.f,
    -1.f, -1.f,
    1.f, -1.f,
    1.f, 1.f};

static const GLuint elements[] = {
    0, 1, 2,
    2, 3, 0};

static const char *shader_sources[2] = {(const char *)res_vertex_source_data, (const char *)res_fragment_source_data};
static const char *texture_mappings[] = {"ymap", "umap", "vmap"};

static int width, height;

GLuint vbo, ebo;
static GLuint texture_id[3], texture_uniform[3];
static GLuint vertex_shader, fragment_shader, shader_program;

static bool renderer_ready = false, frame_arrived;
RenderQueueSubmit render_queue_submit_sdl = NULL;

static bool renderer_setup_sdl(PSTREAM_CONFIGURATION conf, RenderQueueSubmit queueSubmit)
{
    renderer_ready = false;
    frame_arrived = false;
    width = conf->width;
    height = conf->height;
    render_queue_submit_sdl = queueSubmit;

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    int status;
    char msg[512];
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &shader_sources[0], (const int *)&res_vertex_source_size);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buf[1024];
        GLsizei len;
        glGetShaderInfoLog(vertex_shader, 1024, &len, buf);
        fprintf(stderr, "Vertex shader compile error:%.*s\n", len, buf);
    }
    assert(status == GL_TRUE);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &shader_sources[1], (const int *)&res_fragment_source_size);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buf[1024];
        GLsizei len;
        glGetShaderInfoLog(fragment_shader, 1024, &len, buf);
        fprintf(stderr, "Fragment shader compile error:%.*s\n", len, buf);
    }
    assert(status == GL_TRUE);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);

    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    glBindAttribLocation(shader_program, 0, "position");

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
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    renderer_ready = true;
    return true;
}

bool renderer_submit_frame(AVFrame *frame)
{
    if (pthread_mutex_lock(&mutex_ffsw) == 0)
    {
        if (!renderer_ready || !frame)
        {
            pthread_mutex_unlock(&mutex_ffsw);
            return false;
        }
        uint8_t **image = frame->data;

        for (int i = 0; i < 3; i++)
        {
            glBindTexture(GL_TEXTURE_2D, texture_id[i]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, i > 0 ? width / 2 : width, i > 0 ? height / 2 : height, GL_LUMINANCE, GL_UNSIGNED_BYTE, image[i]);
        }
        frame_arrived |= true;
        pthread_mutex_unlock(&mutex_ffsw);
    }
    return true;
}

bool renderer_draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, 0, 0, 0);

    if (!frame_arrived || !renderer_ready)
    {
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glUseProgram(shader_program);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

    for (int i = 0; i < 3; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texture_id[i]);
        glUniform1i(texture_uniform[i], i);
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glUseProgram(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

static void renderer_cleanup_sdl()
{
    renderer_ready = false;
    render_queue_submit_sdl = NULL;
    glDeleteProgram(shader_program);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(3, texture_id);
}

VIDEO_RENDER_CALLBACKS render_callbacks_sdl = {
    renderer_setup_sdl,
    (RenderSubmit)renderer_submit_frame,
    renderer_draw,
    renderer_cleanup_sdl,
};