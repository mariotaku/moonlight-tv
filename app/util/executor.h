#pragma once

typedef struct executor_t executor_t;

typedef struct executor_task_t executor_task_t;

typedef int (*executor_action_cb)(void *arg);

typedef void (*executor_cleanup_cb)(void *arg, int result);

typedef void (*executor_finalize_cb)(executor_t *executor);

executor_t *executor_create(const char *name, executor_finalize_cb finalize_fn);

void executor_destroy(executor_t *executor, int wait);

const executor_task_t *executor_execute(executor_t *executor, executor_action_cb action, executor_cleanup_cb finalize,
                                        void *arg);

/**
 *
 * @param executor
 * @param task Task to cancel. If null, all tasks will be cancelled
 */
void executor_cancel(executor_t *executor, const executor_task_t *task);

void *executor_get_userdata(executor_t *executor);

void executor_set_userdata(executor_t *executor, void *userdata);

int executor_is_cancelled(const executor_t *executor, const executor_task_t *task);

int executor_is_active(const executor_t *executor);

int executor_is_destroyed(const executor_t *executor);