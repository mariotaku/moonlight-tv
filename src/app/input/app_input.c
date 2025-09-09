#include "app_input.h"
#include "logging.h"
#include "app.h"
#include "input_gamepad_mapping.h"
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include "evdev_mouse.h"
#include <string.h>
#include "lvgl/lv_sdl_drv_input.h"

#define MAX_GRAB_FDS 32
static int s_grab_fds[MAX_GRAB_FDS];
static int s_grab_fds_count = 0;

static int is_mouse_like_device(int fd) {
    unsigned long ev_bits[(EV_MAX + 8*sizeof(unsigned long) - 1)/(8*sizeof(unsigned long))];
    unsigned long rel_bits[(REL_MAX + 8*sizeof(unsigned long) - 1)/(8*sizeof(unsigned long))];
    unsigned long key_bits[(KEY_MAX + 8*sizeof(unsigned long) - 1)/(8*sizeof(unsigned long))];
    memset(ev_bits, 0, sizeof(ev_bits));
    memset(rel_bits, 0, sizeof(rel_bits));
    memset(key_bits, 0, sizeof(key_bits));

    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) return 0;

    if (!(ev_bits[EV_REL / (8*sizeof(unsigned long))] & (1UL << (EV_REL % (8*sizeof(unsigned long)))))) {
        return 0;
    }
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits) < 0) return 0;
    int has_rel_xy = (rel_bits[REL_X / (8*sizeof(unsigned long))] & (1UL << (REL_X % (8*sizeof(unsigned long))))) &&
                     (rel_bits[REL_Y / (8*sizeof(unsigned long))] & (1UL << (REL_Y % (8*sizeof(unsigned long)))));
    if (!has_rel_xy) return 0;

    if (!(ev_bits[EV_KEY / (8*sizeof(unsigned long))] & (1UL << (EV_KEY % (8*sizeof(unsigned long)))))) {
        return 0;
    }
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) return 0;

    int btn_mouse_index = BTN_MOUSE; // 0x110
    int has_btn_mouse =
        (key_bits[btn_mouse_index / (8*sizeof(unsigned long))] & (1UL << (btn_mouse_index % (8*sizeof(unsigned long)))));

    return has_btn_mouse;
}

static void webos_grab_mice(int enable) {
    for (int i = 0; i < s_grab_fds_count; ++i) {
        if (s_grab_fds[i] >= 0) {
            int zero = 0;
            ioctl(s_grab_fds[i], EVIOCGRAB, &zero);
            close(s_grab_fds[i]);
            s_grab_fds[i] = -1;
        }
    }
    s_grab_fds_count = 0;

    if (!enable) return;

    DIR *dir = opendir("/dev/input");
    if (!dir) return;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL && s_grab_fds_count < MAX_GRAB_FDS) {
        if (strncmp(de->d_name, "event", 5) != 0) continue;
        char path[128];
        snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        if (is_mouse_like_device(fd)) {
            int one = 1;
            if (ioctl(fd, EVIOCGRAB, &one) == 0) {
                s_grab_fds[s_grab_fds_count++] = fd;
                continue;
            }
        }
        close(fd);
    }
    closedir(dir);
}

void app_input_init(app_input_t *input, app_t *app) {
    input->app = app;
    if (app->settings.condb_path != NULL) {
        app_input_copy_initial_gamepad_mapping(&app->settings);
#if SDL_VERSION_ATLEAST(2, 0, 10)
        SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, app->settings.condb_path);
#endif
    }
    SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    input->max_num_gamepads = 4;
    input->gamepads_count = 0;
    for (int i = 0; i < input->max_num_gamepads; i++) {
        input->gamepads[i].instance_id = -1;
        input->gamepads[i].gs_id = -1;
    }
    input->activeGamepadMask = 0;
    input->blank_cursor_surface = SDL_CreateRGBSurface(0, 16, 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    input->blank_cursor_surface->userdata = SDL_CreateColorCursor(input->blank_cursor_surface, 0, 0);
    if (input->blank_cursor_surface->userdata == NULL) {
        commons_log_warn("Input", "Failed to create blank cursor: %s", SDL_GetError());
    }
#if !SDL_VERSION_ATLEAST(2, 0, 10)
    SDL_GameControllerAddMappingsFromFile(app->settings.condb_path);
#endif
    app_input_init_gamepad_mapping(input, app->backend.executor, &app->settings);
}

void app_input_deinit(app_input_t *input) {
    app_input_deinit_gamepad_mapping(input);
    if (input->blank_cursor_surface->userdata != NULL) {
        SDL_FreeCursor(input->blank_cursor_surface->userdata);
    }
    SDL_FreeSurface(input->blank_cursor_surface);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    evdev_mouse_stop();

}


void app_set_mouse_grab(app_input_t *input, bool grab) {
#ifdef TARGET_WEBOS
    if (grab) evdev_mouse_start();
    else      evdev_mouse_stop();
#endif

    SDL_CaptureMouse(grab ? SDL_TRUE : SDL_FALSE);

    if (!app_configuration->hardware_mouse) {
        SDL_SetRelativeMouseMode(grab && !app_configuration->absmouse ? SDL_TRUE : SDL_FALSE);
    }

#ifdef TARGET_WEBOS
    // Evita cortar aquí para poder ocultar cursor SDL también
    // if (input->app->os_info.version.major >= 9) { return; }
#endif
    SDL_ShowCursor(grab ? SDL_FALSE : SDL_TRUE);
}
bool app_get_mouse_relative() {
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}
