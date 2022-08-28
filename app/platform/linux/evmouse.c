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

static int mouse_fd_default();

static bool is_mouse(int fd, mouse_info_t *info);

static inline bool has_bit(const uint8_t *bits, uint32_t bit);

struct evmouse_t {
    int fd;
};

evmouse_t *evmouse_open_default() {
    evmouse_t *mouse = malloc(sizeof(evmouse_t));
    memset(mouse, 0, sizeof(evmouse_t));
    mouse->fd = mouse_fd_default();
    if (mouse->fd < 0) {
        free(mouse);
        return NULL;
    }
    return mouse;
}

void evmouse_close(evmouse_t *mouse) {
    assert(mouse != NULL);
    assert(mouse->fd >= 0);
    close(mouse->fd);
    free(mouse);
}

static int mouse_fd_default() {
    DIR *dir = opendir("/dev/input");
    if (dir == NULL) {
        return -1;
    }
    struct dirent *ent;
    char dev_path[32] = "/dev/input/";
    while ((ent = readdir(dir))) {
        if (strncmp(ent->d_name, "event", 5) != 0) {
            continue;
        }
        strncpy(&dev_path[11], ent->d_name, 20);
        dev_path[31] = '\0';
        int fd = open(dev_path, O_RDONLY);
        if (fd < 0) {
            applog_w("EvMouse", "Failed to open %s: %s", dev_path, strerror(errno));
            continue;
        }
        mouse_info_t mouse_info;
        if (is_mouse(fd, &mouse_info)) {
            char name[256];
            if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
                applog_i("EvMouse", "Device %s is a mouse: %s", dev_path, name);
            }
            return fd;
        }
        close(fd);
    }
    closedir(dir);
    return -1;
}

static bool is_mouse(int fd, mouse_info_t *info) {
    if (ioctl(fd, EVIOCGID, &info->id) < 0) {
        return false;
    }

    uint8_t keycaps[(KEY_MAX / 8) + 1];
    uint8_t relcaps[(REL_MAX / 8) + 1];
    uint8_t abscaps[(ABS_MAX / 8) + 1];

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

static inline bool has_bit(const uint8_t *bits, uint32_t bit) {
    return (bits[bit / 8] & 1 << (bit % 8)) != 0;
}