#include "evmouse.h"

#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <dirent.h>
#include <printf.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/input.h>

#include "util/logging.h"

typedef struct mouse_info_t {
    struct input_id id;
} mouse_info_t;

#define MOUSE_MAX_FDS 8

typedef bool (*mouse_filter_fn)(const mouse_info_t *info);

static int mouse_fds_find(int *fds, int max, mouse_filter_fn filter);

static bool mouse_filter_any(const mouse_info_t *info);

static bool is_mouse(int fd, mouse_info_t *info);

static inline bool has_bit(const uint8_t *bits, uint32_t bit);

static void dispatch_motion(const struct input_event *raw, evmouse_listener_t listener, void *userdata);

static void dispatch_wheel(const struct input_event *raw, evmouse_listener_t listener, void *userdata);

static void dispatch_button(const struct input_event *raw, evmouse_listener_t listener, void *userdata);

struct evmouse_t {
    int fds[MOUSE_MAX_FDS];
    int nfds;
    bool listening;
};

evmouse_t *evmouse_open_default() {
    evmouse_t *mouse = malloc(sizeof(evmouse_t));
    memset(mouse, 0, sizeof(evmouse_t));
    mouse->nfds = mouse_fds_find(mouse->fds, MOUSE_MAX_FDS, mouse_filter_any);
    if (mouse->nfds <= 0) {
        free(mouse);
        return NULL;
    }
    return mouse;
}

void evmouse_close(evmouse_t *mouse) {
    assert(mouse != NULL);
    for (int i = 0; i < mouse->nfds; i++) {
        close(mouse->fds[i]);
    }
    free(mouse);
}

void evmouse_listen(evmouse_t *mouse, evmouse_listener_t listener, void *userdata) {
    mouse->listening = true;
    while (mouse->listening) {
        fd_set fds;
        FD_ZERO(&fds);
        for (int i = 0; i < mouse->nfds; i++) {
            FD_SET(mouse->fds[i], &fds);
        }
        struct timeval timeout = {.tv_sec = 0, .tv_usec = 1000};
        if (select(FD_SETSIZE, &fds, NULL, NULL, NULL) <= 0) {
            continue;
        }

        for (int i = 0; i < mouse->nfds; i++) {
            int fd = mouse->fds[i];
            if (!FD_ISSET(fd, &fds)) {
                continue;
            }
            struct input_event raw_ev;
            if (read(fd, &raw_ev, sizeof(raw_ev)) != sizeof(raw_ev)) {
                continue;
            }
            switch (raw_ev.type) {
                case EV_REL:
                    switch (raw_ev.code) {
                        case REL_X:
                        case REL_Y:
                            dispatch_motion(&raw_ev, listener, userdata);
                            break;
                        case REL_HWHEEL:
                        case REL_WHEEL:
                            dispatch_wheel(&raw_ev, listener, userdata);
                            break;
                    }
                    break;
                case EV_KEY:
                    switch (raw_ev.code) {
                        case BTN_LEFT:
                        case BTN_RIGHT:
                        case BTN_MIDDLE:
                            dispatch_button(&raw_ev, listener, userdata);
                            break;
                    }
                    break;
            }
        }
    }
}

void evmouse_interrupt(evmouse_t *mouse) {
    mouse->listening = false;
}

static int mouse_fds_find(int *fds, int max, mouse_filter_fn filter) {
    DIR *dir = opendir("/dev/input");
    if (dir == NULL) {
        return 0;
    }
    struct dirent *ent;
    char dev_path[32] = "/dev/input/";
    int nfds = 0;
    while (nfds < max && (ent = readdir(dir))) {
        if (strncmp(ent->d_name, "event", 5) != 0) {
            continue;
        }
        strncpy(&dev_path[11], ent->d_name, 20);
        dev_path[31] = '\0';
        int fd = open(dev_path, O_RDONLY);
        if (fd < 0) {
            // Silently ignore "No such device or address"
            if (errno != ENXIO) {
                applog_w("EvMouse", "Failed to open %s: %d (%s)", dev_path, errno, strerror(errno));
            }
            continue;
        }
        mouse_info_t mouse_info;
        if (is_mouse(fd, &mouse_info) && (!filter || filter(&mouse_info))) {
            fds[nfds++] = fd;
            continue;
        }
        close(fd);
    }
    closedir(dir);
    return nfds;
}

static bool is_mouse(int fd, mouse_info_t *info) {
    if (ioctl(fd, EVIOCGID, &info->id) < 0) {
        return false;
    }

    uint8_t keycaps[(KEY_MAX / 8) + 1];
    uint8_t relcaps[(REL_MAX / 8) + 1];

    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keycaps)), keycaps) < 0) {
        return false;
    }

    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relcaps)), relcaps) < 0) {
        return false;
    }
    if (has_bit(keycaps, BTN_MOUSE) && has_bit(relcaps, REL_X) && has_bit(relcaps, REL_Y)) {
        return true;
    }

    return false;
}

static bool mouse_filter_any(const mouse_info_t *info) {
    return true;
}

static inline bool has_bit(const uint8_t *bits, uint32_t bit) {
    return (bits[bit / 8] & 1 << (bit % 8)) != 0;
}

static void dispatch_motion(const struct input_event *raw, evmouse_listener_t listener, void *userdata) {
    evmouse_event_t event = {.motion = {.type=SDL_MOUSEMOTION}};
    switch (raw->code) {
        case REL_X:
            event.motion.xrel = raw->value;
            break;
        case REL_Y:
            event.motion.yrel = raw->value;
            break;
    }
    listener(&event, userdata);
}

static void dispatch_wheel(const struct input_event *raw, evmouse_listener_t listener, void *userdata) {
    evmouse_event_t event = {.wheel = {.type=SDL_MOUSEWHEEL}};
    switch (raw->code) {
        case REL_WHEEL:
            event.wheel.y = raw->value;
            break;
        case REL_HWHEEL:
            event.wheel.x = raw->value;
            break;
    }
    listener(&event, userdata);
}

static void dispatch_button(const struct input_event *raw, evmouse_listener_t listener, void *userdata) {
    evmouse_event_t event = {
            .button = {
                    .type=raw->value ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP,
                    .state = raw->value ? SDL_PRESSED : SDL_RELEASED
            }
    };
    switch (raw->code) {
        case BTN_LEFT:
            event.button.button = SDL_BUTTON_LEFT;
            break;
        case BTN_RIGHT:
            event.button.button = SDL_BUTTON_RIGHT;
            break;
        case BTN_MIDDLE:
            event.button.button = SDL_BUTTON_MIDDLE;
            break;
    }
    listener(&event, userdata);
}