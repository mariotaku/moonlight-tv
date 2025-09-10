#include "diag.fragment.h"

lv_obj_t *diag_win_create(lv_fragment_t *self, lv_obj_t *parent);

void diag_win_deleted(lv_fragment_t *self, lv_obj_t *obj);

static int ping_router(void *arg);

static void diag_controller(lv_fragment_t *self, void *args) {
    diag_fragment_t *f = (diag_fragment_t *) self;
    f->executor = executor_create("moonlight-diag", 1);
}

static void diag_dtor(lv_fragment_t *self) {
    diag_fragment_t *f = (diag_fragment_t *) self;
    executor_destroy(f->executor);
}

static void diag_obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    diag_fragment_t *f = (diag_fragment_t *) self;
    executor_submit(f->executor, ping_router, NULL, self);
}

static int ping_router(void *arg) {
    // run ping -c 10, and parse the output
    FILE *pipe = popen("ping -c 2 192.168.89.1", "r");
    if (!pipe) {
        return -1;
    }
    char buffer[1024];
    size_t buf_len = fread(buffer, 1, sizeof(buffer) - 1, pipe);
    pclose(pipe);
    buffer[buf_len] = '\0';
    int transmitted = 0, received = 0, errors = 0, time_ms = 0;
    char* ptr = buffer;
    // find the first line after `ping statistics ---`
    ptr = strstr(ptr, "ping statistics ---\n");
    if (!ptr) {
        return -1;
    }
    // move to the next line
    ptr += strlen("ping statistics ---\n");
    if (sscanf(ptr, "%d packets transmitted, %d received, %*d%% packet loss, time %dms", &transmitted, &received, &time_ms) != 4) {
        sscanf(ptr, "%d packets transmitted, %d received, %d errors, time %dms", &transmitted, &received, &errors, &time_ms);
    }



    return 0;
}

const lv_fragment_class_t diag_fragment_class = {
    .constructor_cb = diag_controller,
    .destructor_cb = diag_dtor,
    .create_obj_cb = diag_win_create,
    .obj_created_cb = diag_obj_created,
    .obj_deleted_cb = diag_win_deleted,
    .instance_size = sizeof(diag_fragment_t),
};

