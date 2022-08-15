#pragma once

typedef struct executor_t executor_t;

typedef struct executor_task_t executor_task_t;

typedef void (*executor_action_cb)(void *arg);

typedef void (*executor_cleanup_cb)(void *arg, int cancelled);

typedef void (*executor_free_cb)(executor_t *executor);

executor_t *executor_create(const char *name, executor_free_cb free_fn);

void executor_destroy(executor_t *executor, int wait);

const executor_task_t *executor_execute(executor_t *executor, executor_action_cb action, executor_cleanup_cb finalize,
                                        void *arg);

void executor_cancel(executor_t *executor, const executor_task_t *task);

void *executor_get_userdata(executor_t *executor);

void executor_set_userdata(executor_t *executor, void *userdata);

int executor_is_cancelled(executor_t *executor, const executor_task_t *task);

int executor_is_active(executor_t *executor);
