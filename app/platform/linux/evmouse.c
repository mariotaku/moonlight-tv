#include "evmouse.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <dirent.h>
#include <printf.h>

#include "util/logging.h"

static int mouse_fd_default();

struct evmouse_t {
    int fd;
};

evmouse_t *evmouse_open_default() {
    evmouse_t *mouse = malloc(sizeof(evmouse_t));
    memset(mouse, 0, sizeof(evmouse_t));
    mouse_fd_default();
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
        strncpy(&dev_path[11], ent->d_name, 20);
        dev_path[31] = '\0';
        applog_i("EvMouse", "read device %s", dev_path);
    }
    closedir(dir);
    return 0;
}