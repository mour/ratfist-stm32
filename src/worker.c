/**
 * @file
 *
 * TODO Add file description
 */

#include "worker.h"


static void worker_task_func(void *worker_state)
{
	struct worker *state = worker_state;

	while (!state->stop_flag) {
		state->action(state->params);
	}
}

bool worker_task_init(worker_t *worker,
                      const char *name,
                      uint8_t *stack_base,
                      uint32_t stack_size,
                      uint8_t priority,
                      void (*action)(void *),
                      void *action_params)
{
	worker->action = action;
	worker->params = action_params;

	return os_task_init(&(worker->task),
	                    name, stack_base, stack_size, priority,
	                    worker_task_func, worker);
}


bool worker_start(worker_t *worker)
{
	worker->stop_flag = false;
	return os_task_add(&(worker->task));
}


void worker_stop(worker_t *worker)
{
	worker->stop_flag = true;
}

void worker_join(worker_t *worker)
{
	while (worker->task.state != TASK_STOPPED);
}
