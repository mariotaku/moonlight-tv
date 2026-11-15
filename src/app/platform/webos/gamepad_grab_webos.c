/*
 * Keep webOS from reacting to gamepad buttons while streaming.
 *
 * Since webOS 26 the system opens the LG Gaming Portal when View/Select or
 * Menu/Start is pressed, even inside Moonlight ("cloudgame_active" does not
 * help). Taking an exclusive EVIOCGRAB on the controller's evdev nodes hides
 * them from the platform input stack.
 *
 * A grab also hides the node from every other reader, including SDL if SDL
 * happens to read that controller through evdev. So a controller is grabbed
 * only when SDL reads it through another channel (HIDAPI/hidraw or a webOS
 * specific backend), i.e. when its SDL device path is not an evdev node.
 * Controllers SDL reads via evdev are left alone: they keep working, the
 * Portal may still open with them.
 */
#include "gamepad_grab_webos.h"

#include <SDL.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_EVENT_DEVICES 64
#define MAX_CONTROLLERS 16
/* Rescan only when the set of controllers changes. Evdev nodes may show up a bit after SDL
 * sees the controller, so keep rescanning for a short while after a change. */
#define RESCAN_INTERVAL_MS 500
#define RESCAN_WINDOW_MS 3000

typedef struct {
    Uint16 vendor, product;
} usb_id_t;

static int grab_fds[MAX_EVENT_DEVICES];
static bool initialized;
static Uint32 next_scan;
static Uint32 scan_until;
static int last_joystick_count = -1;

static void release_all(void) {
    for (int i = 0; i < MAX_EVENT_DEVICES; ++i) {
        if (grab_fds[i] >= 0) {
            ioctl(grab_fds[i], EVIOCGRAB, 0);
            close(grab_fds[i]);
            grab_fds[i] = -1;
            SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Released gamepad grab on /dev/input/event%d", i);
        }
    }
}

static bool id_in(const usb_id_t *list, int count, Uint16 vendor, Uint16 product) {
    for (int i = 0; i < count; ++i) {
        if (list[i].vendor == vendor && list[i].product == product) {
            return true;
        }
    }
    return false;
}

static void id_add(usb_id_t *list, int *count, Uint16 vendor, Uint16 product) {
    if (*count < MAX_CONTROLLERS && !id_in(list, *count, vendor, product)) {
        list[*count].vendor = vendor;
        list[*count].product = product;
        (*count)++;
    }
}

static bool read_hex_file(const char *path, unsigned *value) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return false;
    }
    bool ok = fscanf(fp, "%x", value) == 1;
    fclose(fp);
    return ok;
}

/*
 * Read the id of /dev/input/eventN from sysfs. Opening evdev nodes can take hundreds of
 * milliseconds on webOS, so only nodes that match are opened.
 * Returns 1 if read, 0 if the node doesn't exist, -1 if unknown (caller falls back to open).
 */
static int read_node_id(int index, Uint16 *vendor, Uint16 *product) {
    char path[96];
    struct stat st;
    snprintf(path, sizeof(path), "/sys/class/input/event%d", index);
    if (stat(path, &st) != 0) {
        return 0;
    }
    unsigned v, p;
    snprintf(path, sizeof(path), "/sys/class/input/event%d/device/id/vendor", index);
    if (!read_hex_file(path, &v)) {
        return -1;
    }
    snprintf(path, sizeof(path), "/sys/class/input/event%d/device/id/product", index);
    if (!read_hex_file(path, &p)) {
        return -1;
    }
    *vendor = (Uint16) v;
    *product = (Uint16) p;
    return 1;
}

/* Build the list of controllers that are safe to grab. */
static int collect_grabbable(usb_id_t *grab) {
    usb_id_t evdev_read[MAX_CONTROLLERS];
    int n_grab = 0, n_evdev = 0;
    for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
        if (!SDL_IsGameController(i)) {
            continue;
        }
        Uint16 vendor = SDL_JoystickGetDeviceVendor(i), product = SDL_JoystickGetDeviceProduct(i);
        if (vendor == 0 && product == 0) {
            continue;
        }
        if (vendor == 0x9998 && product == 0x9998) {
            /* webOS virtual Smart Remote */
            continue;
        }
        const char *path = SDL_JoystickPathForIndex(i);
        if (path != NULL && strncmp(path, "/dev/input/event", 16) == 0) {
            id_add(evdev_read, &n_evdev, vendor, product);
        } else {
            id_add(grab, &n_grab, vendor, product);
        }
    }
    /* If any controller with this id is read through evdev, never grab that id. */
    int out = 0;
    for (int i = 0; i < n_grab; ++i) {
        if (!id_in(evdev_read, n_evdev, grab[i].vendor, grab[i].product)) {
            grab[out++] = grab[i];
        }
    }
    return out;
}

void webos_gamepad_grab_update(bool active) {
    if (!initialized) {
        for (int i = 0; i < MAX_EVENT_DEVICES; ++i) {
            grab_fds[i] = -1;
        }
        initialized = true;
    }

    if (!active) {
        release_all();
        last_joystick_count = -1;
        return;
    }

    Uint32 now = SDL_GetTicks();
    int joystick_count = SDL_NumJoysticks();
    if (joystick_count != last_joystick_count) {
        last_joystick_count = joystick_count;
        scan_until = now + RESCAN_WINDOW_MS;
        next_scan = now;
    }
    if (SDL_TICKS_PASSED(now, scan_until) || !SDL_TICKS_PASSED(now, next_scan)) {
        return;
    }
    next_scan = now + RESCAN_INTERVAL_MS;

    usb_id_t grab[MAX_CONTROLLERS];
    int n_grab = collect_grabbable(grab);

    for (int i = 0; i < MAX_EVENT_DEVICES; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        if (grab_fds[i] >= 0) {
            struct stat path_stat, fd_stat;
            struct input_id id;
            bool same_node = stat(path, &path_stat) == 0 && fstat(grab_fds[i], &fd_stat) == 0 &&
                             path_stat.st_dev == fd_stat.st_dev && path_stat.st_ino == fd_stat.st_ino;
            bool still_wanted = ioctl(grab_fds[i], EVIOCGID, &id) == 0 &&
                                id_in(grab, n_grab, id.vendor, id.product);
            if (same_node && still_wanted) {
                continue;
            }
            ioctl(grab_fds[i], EVIOCGRAB, 0);
            close(grab_fds[i]);
            grab_fds[i] = -1;
        }

        if (n_grab == 0) {
            continue;
        }
        Uint16 vendor, product;
        int known = read_node_id(i, &vendor, &product);
        if (known == 0 || (known == 1 && !id_in(grab, n_grab, vendor, product))) {
            continue;
        }
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        struct input_id id;
        if (ioctl(fd, EVIOCGID, &id) == 0 && id_in(grab, n_grab, id.vendor, id.product) &&
            ioctl(fd, EVIOCGRAB, 1) == 0) {
            grab_fds[i] = fd;
            SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Grabbed gamepad %04x:%04x at %s", id.vendor, id.product, path);
            continue;
        }
        close(fd);
    }
}
