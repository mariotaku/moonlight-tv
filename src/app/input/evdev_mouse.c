#include "evdev_mouse.h"


#include <SDL.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

#define MAX_MICE 8

+#define BITS_PER_LONG (sizeof(unsigned long) * 8)

static int s_fds[MAX_MICE];
static int s_fd_count = 0;
static pthread_t s_thread;
static atomic_bool s_running = false;

static int is_mouse_like_device(int fd) {
    unsigned long ev[(EV_MAX+BITS_PER_LONG-1)/BITS_PER_LONG];
    unsigned long rel[(REL_MAX+BITS_PER_LONG-1)/BITS_PER_LONG];
    unsigned long key[(KEY_MAX+BITS_PER_LONG-1)/BITS_PER_LONG];
    memset(ev,0,sizeof(ev)); memset(rel,0,sizeof(rel)); memset(key,0,sizeof(key));
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev)), ev) < 0) return 0;

    // Necesitamos eventos relativos y teclas de mouse
    if (!(ev[EV_REL/BITS_PER_LONG] & (1UL << (EV_REL % BITS_PER_LONG)))) return 0;
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel) < 0) return 0;
    int has_xy = (rel[REL_X/BITS_PER_LONG] & (1UL << (REL_X % BITS_PER_LONG))) &&
                 (rel[REL_Y/BITS_PER_LONG] & (1UL << (REL_Y % BITS_PER_LONG)));
    if (!has_xy) return 0;

    if (!(ev[EV_KEY/BITS_PER_LONG] & (1UL << (EV_KEY % BITS_PER_LONG)))) return 0;
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key)), key) < 0) return 0;

    int btn = BTN_MOUSE; // 0x110
    int has_btn_mouse = key[btn/BITS_PER_LONG] & (1UL << (btn % BITS_PER_LONG));
    return has_btn_mouse;
}

static void release_all(void) {
    for (int i = 0; i < s_fd_count; ++i) {
        if (s_fds[i] >= 0) {
            int zero = 0;
            ioctl(s_fds[i], EVIOCGRAB, &zero);
            close(s_fds[i]);
            s_fds[i] = -1;
        }
    }
    s_fd_count = 0;
}

static int open_and_grab_mice(void) {
    release_all();
    DIR *dir = opendir("/dev/input");
    if (!dir) return -1;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL && s_fd_count < MAX_MICE) {
        if (strncmp(de->d_name, "event", 5) != 0) continue;
        char path[128]; snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        if (is_mouse_like_device(fd)) {
            int one = 1;
            if (ioctl(fd, EVIOCGRAB, &one) == 0) {
                s_fds[s_fd_count++] = fd;
                continue;
            }
        }
        close(fd);
    }
    closedir(dir);
    return s_fd_count;
}

static void push_motion(int dx, int dy) {
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEMOTION;
    e.motion.type = SDL_MOUSEMOTION;
    e.motion.state = 0;
    e.motion.xrel = dx;
    e.motion.yrel = dy;
    // x/y (absolutos) los dejamos como 0: SDL los acumula en relativo si está en modo relativo
    SDL_PushEvent(&e);
}

static void push_button(uint8_t sdl_button, int pressed) {
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    e.button.type = e.type;
    e.button.button = sdl_button;
    e.button.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    SDL_PushEvent(&e);
}

static void push_wheel(int dx, int dy) {
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEWHEEL;
    e.wheel.type = SDL_MOUSEWHEEL;
    e.wheel.x = dx;
    e.wheel.y = dy;
    e.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
    SDL_PushEvent(&e);
}

static uint8_t map_btn(int code) {
    switch (code) {
        case BTN_LEFT:   return SDL_BUTTON_LEFT;
        case BTN_RIGHT:  return SDL_BUTTON_RIGHT;
        case BTN_MIDDLE: return SDL_BUTTON_MIDDLE;
        // opcionales:
        // case BTN_SIDE:   return SDL_BUTTON_X1;
        // case BTN_EXTRA:  return SDL_BUTTON_X2;
        default: return 0;
    }
}

static void* thread_fn(void *_) {
    (void)_;
    struct pollfd pfds[MAX_MICE];

    while (s_running) {
        if (s_fd_count == 0) {
            if (open_and_grab_mice() <= 0) {
                // No hay ratones, espera un poco
                SDL_Delay(250);
                continue;
            }
        }
        for (int i = 0; i < s_fd_count; ++i) {
            pfds[i].fd = s_fds[i];
            pfds[i].events = POLLIN;
            pfds[i].revents = 0;
        }
        int pr = poll(pfds, s_fd_count, 50);
        if (pr <= 0) continue;

        for (int i = 0; i < s_fd_count; ++i) {
            if (!(pfds[i].revents & POLLIN)) continue;
            struct input_event ev[32];
            ssize_t rd;
            while ((rd = read(pfds[i].fd, ev, sizeof(ev))) > 0) {
                int count = rd / sizeof(struct input_event);
                int dx = 0, dy = 0, wx = 0, wy = 0;
                for (int k = 0; k < count; ++k) {
                    if (ev[k].type == EV_REL) {
                        switch (ev[k].code) {
                            case REL_X: dx += ev[k].value; break;
                            case REL_Y: dy += ev[k].value; break;
                            case REL_WHEEL: wy += ev[k].value; break;
                            case REL_HWHEEL: wx += ev[k].value; break;
                        }
                    } else if (ev[k].type == EV_KEY) {
                        uint8_t b = map_btn(ev[k].code);
                        if (b) push_button(b, ev[k].value ? 1 : 0);
                    } else if (ev[k].type == EV_SYN && ev[k].code == SYN_REPORT) {
                        if (dx || dy) { push_motion(dx, dy); dx = dy = 0; }
                        if (wx || wy) { push_wheel(wx, wy); wx = wy = 0; }
                    }
                }
            }
            if (rd < 0 && errno != EAGAIN && errno != EINTR) {
                // Dispositivo desconectado? Reabrir en la próxima vuelta
                release_all();
                break;
            }
        }
    }

    release_all();
    return NULL;
}

bool evdev_mouse_start(void) {
    if (s_running) return true;
    memset(s_fds, -1, sizeof(s_fds));
    s_fd_count = 0;
    s_running = true;
    if (pthread_create(&s_thread, NULL, thread_fn, NULL) != 0) {
        s_running = false;
        return false;
    }
    return true;
}

void evdev_mouse_stop(void) {
    if (!s_running) return;
    s_running = false;
    pthread_join(s_thread, NULL);
    // release_all() ya se llama al salir del hilo
}

