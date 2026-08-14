#include "stream/input/session_input.h"

#include <SDL_touch.h>

static void stream_input_send_touch(const SDL_TouchFingerEvent *event) {
    uint8_t type;
    switch (event->type) {
        case SDL_FINGERDOWN:
            type = LI_TOUCH_EVENT_DOWN;
            break;
        case SDL_FINGERUP:
            type = LI_TOUCH_EVENT_UP;
            break;
        case SDL_FINGERMOTION:
            type = LI_TOUCH_EVENT_MOVE;
            break;
        default:
            return;
    }
    LiSendTouchEvent(type, event->fingerId, event->x, event->y, event->pressure, 0, 0, 0);
}

#if TARGET_WEBOS
#define CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES 8
#define CONTROLLER_TOUCHPAD_COMPAT_EVENT_WINDOW_MS 50u

enum {
    CONTROLLER_TOUCHPAD_COMPAT_UNUSED = 0,
    CONTROLLER_TOUCHPAD_COMPAT_PENDING,
    CONTROLLER_TOUCHPAD_COMPAT_FORWARDED,
    CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED,
};

enum {
    CONTROLLER_TOUCHPAD_COMPAT_HAS_MOTION = 1u << 0,
    CONTROLLER_TOUCHPAD_COMPAT_HAS_UP = 1u << 1,
};

typedef struct controller_touchpad_compat_sample_t {
    float x, y, pressure;
} controller_touchpad_compat_sample_t;

typedef struct controller_touchpad_compat_candidate_t {
    SDL_TouchID touch_id;
    SDL_FingerID finger_id;
    uint32_t down_timestamp;
    controller_touchpad_compat_sample_t down, motion, up;
    uint8_t state, buffered;
} controller_touchpad_compat_candidate_t;

struct session_input_controller_touchpad_compat_t {
    bool controller_present;
    bool last_down_valid;
    bool has_pending;
    bool has_suppressed;
    uint32_t last_down_timestamp;
    uint32_t suppress_until;
    controller_touchpad_compat_candidate_t candidates[CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES];
};

static void controller_touchpad_compat_send_sample(uint8_t type, SDL_FingerID finger_id,
                                                   const controller_touchpad_compat_sample_t *sample) {
    LiSendTouchEvent(type, finger_id, sample->x, sample->y, sample->pressure, 0, 0, 0);
}

void stream_input_controller_touchpad_compat_deinit(stream_input_t *input) {
    session_input_controller_touchpad_compat_t *compat = input->controller_touchpad_compat;
    if (compat == NULL) {
        return;
    }

    // A forwarded touchscreen contact already has a DOWN event on the host.
    // Cancel it before discarding compatibility state to avoid a stuck touch.
    for (int i = 0; i < CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES; ++i) {
        controller_touchpad_compat_candidate_t *candidate = &compat->candidates[i];
        if (candidate->state == CONTROLLER_TOUCHPAD_COMPAT_FORWARDED) {
            LiSendTouchEvent(LI_TOUCH_EVENT_CANCEL, candidate->finger_id, 0, 0, 0, 0, 0, 0);
        }
    }

    SDL_free(compat);
    input->controller_touchpad_compat = NULL;
}

void stream_input_controller_touchpad_compat_set_present(stream_input_t *input, bool present) {
    if (present && input->controller_touchpad_compat == NULL) {
        input->controller_touchpad_compat = SDL_calloc(1, sizeof(*input->controller_touchpad_compat));
    }
    if (input->controller_touchpad_compat != NULL) {
        input->controller_touchpad_compat->controller_present = present;
    }
}

static uint32_t controller_touchpad_compat_timestamp_distance(uint32_t a, uint32_t b) {
    uint32_t forward = a - b;
    uint32_t backward = b - a;
    return forward < backward ? forward : backward;
}

static bool controller_touchpad_compat_timestamp_matches(uint32_t a, uint32_t b) {
    return controller_touchpad_compat_timestamp_distance(a, b) <=
           CONTROLLER_TOUCHPAD_COMPAT_EVENT_WINDOW_MS;
}

static bool controller_touchpad_compat_event_queued(uint32_t timestamp) {
    SDL_Event pending[16];
    int count = SDL_PeepEvents(pending, (int) (sizeof(pending) / sizeof(pending[0])),
                               SDL_PEEKEVENT, SDL_CONTROLLERTOUCHPADDOWN, SDL_CONTROLLERTOUCHPADUP);
    for (int i = 0; i < count; ++i) {
        if (pending[i].ctouchpad.touchpad == 0 &&
            controller_touchpad_compat_timestamp_matches(pending[i].ctouchpad.timestamp, timestamp)) {
            return true;
        }
    }
    return false;
}

static bool controller_touchpad_compat_matches_recent_down(
        const session_input_controller_touchpad_compat_t *compat, uint32_t timestamp) {
    return compat->last_down_valid &&
           controller_touchpad_compat_timestamp_matches(compat->last_down_timestamp, timestamp);
}

bool stream_input_controller_touchpad_compat_mouse_active(const stream_input_t *input,
                                                          uint32_t timestamp) {
    if (!input->controller_touchpad_webos_touchscreen) {
        return true;
    }

    const session_input_controller_touchpad_compat_t *compat = input->controller_touchpad_compat;
    if (compat == NULL) {
        return false;
    }

    if (controller_touchpad_compat_matches_recent_down(compat, timestamp) ||
        !SDL_TICKS_PASSED(timestamp, compat->suppress_until)) {
        return true;
    }

    if (compat->has_suppressed) {
        return true;
    }

    return compat->controller_present && controller_touchpad_compat_event_queued(timestamp);
}

static controller_touchpad_compat_candidate_t *controller_touchpad_compat_find_candidate(
        session_input_controller_touchpad_compat_t *compat, SDL_TouchID touch_id, SDL_FingerID finger_id) {
    for (int i = 0; i < CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES; ++i) {
        controller_touchpad_compat_candidate_t *candidate = &compat->candidates[i];
        if (candidate->state != CONTROLLER_TOUCHPAD_COMPAT_UNUSED &&
            candidate->touch_id == touch_id && candidate->finger_id == finger_id) {
            return candidate;
        }
    }
    return NULL;
}

static controller_touchpad_compat_candidate_t *controller_touchpad_compat_alloc_candidate(
        session_input_controller_touchpad_compat_t *compat) {
    for (int i = 0; i < CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES; ++i) {
        if (compat->candidates[i].state == CONTROLLER_TOUCHPAD_COMPAT_UNUSED) {
            return &compat->candidates[i];
        }
    }
    return NULL;
}

static void controller_touchpad_compat_update_activity(
        session_input_controller_touchpad_compat_t *compat) {
    compat->has_pending = false;
    compat->has_suppressed = false;
    for (int i = 0; i < CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES; ++i) {
        switch (compat->candidates[i].state) {
            case CONTROLLER_TOUCHPAD_COMPAT_PENDING:
                compat->has_pending = true;
                break;
            case CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED:
                compat->has_suppressed = true;
                break;
            default:
                break;
        }
        if (compat->has_pending && compat->has_suppressed) {
            return;
        }
    }
}

static bool controller_touchpad_compat_track_down(
        session_input_controller_touchpad_compat_t *compat,
        const SDL_TouchFingerEvent *event, bool suppressed) {
    controller_touchpad_compat_candidate_t *candidate = controller_touchpad_compat_alloc_candidate(compat);
    if (candidate == NULL) {
        return false;
    }

    candidate->touch_id = event->touchId;
    candidate->finger_id = event->fingerId;
    candidate->down_timestamp = event->timestamp;
    candidate->down = (controller_touchpad_compat_sample_t) {event->x, event->y, event->pressure};
    candidate->state = suppressed ? CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED
                                  : CONTROLLER_TOUCHPAD_COMPAT_PENDING;
    candidate->buffered = 0;
    if (suppressed) {
        compat->has_suppressed = true;
    } else {
        compat->has_pending = true;
    }
    return true;
}

static void controller_touchpad_compat_flush_candidate(controller_touchpad_compat_candidate_t *candidate) {
    controller_touchpad_compat_send_sample(LI_TOUCH_EVENT_DOWN, candidate->finger_id, &candidate->down);
    candidate->state = CONTROLLER_TOUCHPAD_COMPAT_FORWARDED;

    if (candidate->buffered & CONTROLLER_TOUCHPAD_COMPAT_HAS_MOTION) {
        controller_touchpad_compat_send_sample(LI_TOUCH_EVENT_MOVE, candidate->finger_id, &candidate->motion);
    }
    if (candidate->buffered & CONTROLLER_TOUCHPAD_COMPAT_HAS_UP) {
        controller_touchpad_compat_send_sample(LI_TOUCH_EVENT_UP, candidate->finger_id, &candidate->up);
        candidate->state = CONTROLLER_TOUCHPAD_COMPAT_UNUSED;
    }
}

void stream_input_controller_touchpad_compat_observe(stream_input_t *input,
                                                     const SDL_ControllerTouchpadEvent *event) {
    session_input_controller_touchpad_compat_t *compat = input->controller_touchpad_compat;
    if (compat == NULL || event->touchpad != 0) {
        return;
    }

    if (event->type == SDL_CONTROLLERTOUCHPADUP) {
        compat->suppress_until = event->timestamp + CONTROLLER_TOUCHPAD_COMPAT_EVENT_WINDOW_MS;
        return;
    }
    if (event->type != SDL_CONTROLLERTOUCHPADDOWN) {
        return;
    }

    compat->last_down_valid = true;
    compat->last_down_timestamp = event->timestamp;

    controller_touchpad_compat_candidate_t *best = NULL;
    uint32_t best_distance = CONTROLLER_TOUCHPAD_COMPAT_EVENT_WINDOW_MS + 1;
    for (int i = 0; i < CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES; ++i) {
        controller_touchpad_compat_candidate_t *candidate = &compat->candidates[i];
        if (candidate->state != CONTROLLER_TOUCHPAD_COMPAT_PENDING &&
            candidate->state != CONTROLLER_TOUCHPAD_COMPAT_FORWARDED) {
            continue;
        }

        uint32_t distance = controller_touchpad_compat_timestamp_distance(
                candidate->down_timestamp, event->timestamp);
        if (distance <= CONTROLLER_TOUCHPAD_COMPAT_EVENT_WINDOW_MS && distance < best_distance) {
            best = candidate;
            best_distance = distance;
        }
    }

    if (best == NULL) {
        return;
    }

    if (best->state == CONTROLLER_TOUCHPAD_COMPAT_PENDING) {
        best->state = (best->buffered & CONTROLLER_TOUCHPAD_COMPAT_HAS_UP)
                      ? CONTROLLER_TOUCHPAD_COMPAT_UNUSED
                      : CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED;
        controller_touchpad_compat_update_activity(compat);
        return;
    }

    LiSendTouchEvent(LI_TOUCH_EVENT_CANCEL, best->finger_id, 0, 0, 0, 0, 0, 0);
    best->state = CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED;
    compat->has_suppressed = true;
}

void stream_input_flush_pending_touch(stream_input_t *input) {
    session_input_controller_touchpad_compat_t *compat = input->controller_touchpad_compat;
    if (compat == NULL || !compat->has_pending) {
        return;
    }

    uint32_t now = SDL_GetTicks();
    compat->has_pending = false;
    for (int i = 0; i < CONTROLLER_TOUCHPAD_COMPAT_TOUCH_CANDIDATES; ++i) {
        controller_touchpad_compat_candidate_t *candidate = &compat->candidates[i];
        if (candidate->state != CONTROLLER_TOUCHPAD_COMPAT_PENDING) {
            continue;
        }
        if (SDL_TICKS_PASSED(now, candidate->down_timestamp +
                                 CONTROLLER_TOUCHPAD_COMPAT_EVENT_WINDOW_MS)) {
            controller_touchpad_compat_flush_candidate(candidate);
        } else {
            compat->has_pending = true;
        }
    }
}
#endif

void stream_input_handle_touch(stream_input_t *input, const SDL_TouchFingerEvent *event) {
#if TARGET_WEBOS
    if (!input->controller_touchpad_webos_touchscreen) {
        return;
    }
#endif

    if (input->view_only) {
        return;
    }

    SDL_TouchDeviceType touch_type = SDL_GetTouchDeviceType(event->touchId);
    if (touch_type == SDL_TOUCH_DEVICE_INDIRECT_ABSOLUTE ||
        touch_type == SDL_TOUCH_DEVICE_INDIRECT_RELATIVE) {
        return;
    }

#if TARGET_WEBOS
    session_input_controller_touchpad_compat_t *compat = input->controller_touchpad_compat;
    controller_touchpad_compat_candidate_t *candidate = compat != NULL
            ? controller_touchpad_compat_find_candidate(compat, event->touchId, event->fingerId)
            : NULL;

    if (event->type == SDL_FINGERDOWN) {
        if (candidate != NULL) {
            bool tracked_activity = candidate->state == CONTROLLER_TOUCHPAD_COMPAT_PENDING ||
                                    candidate->state == CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED;
            if (candidate->state == CONTROLLER_TOUCHPAD_COMPAT_FORWARDED) {
                LiSendTouchEvent(LI_TOUCH_EVENT_CANCEL, candidate->finger_id, 0, 0, 0, 0, 0, 0);
            }
            candidate->state = CONTROLLER_TOUCHPAD_COMPAT_UNUSED;
            if (tracked_activity) {
                controller_touchpad_compat_update_activity(compat);
            }
        }

        if (compat != NULL && compat->controller_present &&
            controller_touchpad_compat_track_down(
                    compat, event, controller_touchpad_compat_matches_recent_down(compat, event->timestamp))) {
            return;
        }
    } else if (candidate != NULL) {
        if (candidate->state == CONTROLLER_TOUCHPAD_COMPAT_SUPPRESSED) {
            if (event->type == SDL_FINGERUP) {
                candidate->state = CONTROLLER_TOUCHPAD_COMPAT_UNUSED;
                controller_touchpad_compat_update_activity(compat);
            }
            return;
        }
        if (candidate->state == CONTROLLER_TOUCHPAD_COMPAT_PENDING) {
            if (event->type == SDL_FINGERMOTION) {
                candidate->motion = (controller_touchpad_compat_sample_t) {event->x, event->y, event->pressure};
                candidate->buffered |= CONTROLLER_TOUCHPAD_COMPAT_HAS_MOTION;
            } else if (event->type == SDL_FINGERUP) {
                candidate->up = (controller_touchpad_compat_sample_t) {event->x, event->y, event->pressure};
                candidate->buffered |= CONTROLLER_TOUCHPAD_COMPAT_HAS_UP;
            }
            return;
        }
    }
#endif

    stream_input_send_touch(event);

#if TARGET_WEBOS
    if (event->type == SDL_FINGERUP && candidate != NULL) {
        candidate->state = CONTROLLER_TOUCHPAD_COMPAT_UNUSED;
    }
#endif
}
