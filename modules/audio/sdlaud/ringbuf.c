#include "ringbuf.h"

#include <SDL.h>

struct sdlaud_ringbuf_t {
    size_t cap, size;
    size_t head, tail;
    SDL_mutex *mutex;
};

sdlaud_ringbuf *sdlaud_ringbuf_new(size_t capacity) {
    sdlaud_ringbuf *buf = SDL_malloc(sizeof(sdlaud_ringbuf) + capacity);
    buf->cap = capacity;
    buf->size = 0;
    buf->head = 0;
    buf->tail = 0;
    buf->mutex = SDL_CreateMutex();
    return buf;
}

size_t sdlaud_ringbuf_write(sdlaud_ringbuf *buf, const unsigned char *src, size_t size) {
    SDL_LockMutex(buf->mutex);
    if (buf->size + size >= buf->cap) {
        // Buffer overflow
        SDL_UnlockMutex(buf->mutex);
        return 0;
    }
    unsigned char *dst = (unsigned char *) buf + sizeof(sdlaud_ringbuf);
    size_t tmp_tail = buf->tail + size;
    if (tmp_tail < buf->cap) {
        // One time read
        SDL_memcpy(&dst[buf->tail], src, size);
    } else {
        size_t tmp_write = buf->cap - buf->tail;
        // Append parts at tail
        SDL_memcpy(&dst[buf->tail], src, tmp_write);
        tmp_tail = size - tmp_write;
        // Append from start
        SDL_memcpy(dst, &src[tmp_write], tmp_tail);
    }
    buf->size += size;
    buf->tail = tmp_tail;

    SDL_UnlockMutex(buf->mutex);
    return size;
}

size_t sdlaud_ringbuf_read(sdlaud_ringbuf *buf, unsigned char *dst, size_t size) {
    SDL_LockMutex(buf->mutex);
    size_t read_size = SDL_min(buf->size, size);
    if (read_size == 0) {
        SDL_UnlockMutex(buf->mutex);
        return 0;
    }
    const unsigned char *src = (unsigned char *) buf + sizeof(sdlaud_ringbuf);
    size_t tmp_head = buf->head + read_size;
    if (tmp_head < buf->cap) {
        // One time read
        SDL_memcpy(dst, &src[buf->head], read_size);
    } else {
        size_t tmp_read = buf->cap - buf->head;
        // Read parts before cap
        SDL_memcpy(dst, &src[buf->head], tmp_read);
        tmp_head = read_size - tmp_read;
        // Read parts from start
        SDL_memcpy(&dst[tmp_read], src, tmp_head);
    }
    buf->size -= read_size;
    buf->head = tmp_head;
    SDL_UnlockMutex(buf->mutex);
    return read_size;
}

size_t sdlaud_ringbuf_clear(sdlaud_ringbuf *buf) {
    SDL_LockMutex(buf->mutex);
    buf->size = 0;
    buf->head = 0;
    buf->tail = 0;
    SDL_UnlockMutex(buf->mutex);
}

size_t sdlaud_ringbuf_size(const sdlaud_ringbuf *buf) {
    return buf->size;
}

void sdlaud_ringbuf_delete(sdlaud_ringbuf *buf) {
    SDL_DestroyMutex(buf->mutex);
    SDL_free(buf);
}