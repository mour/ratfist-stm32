/**
 * @file
 *
 * TODO Add file description.
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <stdint.h>
#include <stdbool.h>

#include <mouros/tasks.h>

typedef struct worker {
	task_t task;
	void (*action)(void *);
	void *params;
	bool stop_flag;
} worker_t;

bool worker_task_init(worker_t *worker,
                      const char *name,
                      uint8_t *stack_base,
                      uint32_t stack_size,
                      uint8_t priority,
                      void (*action)(void *),
                      void *action_params);

bool worker_start(worker_t *worker);
void worker_stop(worker_t *worker);
void worker_join(worker_t *worker);


#endif /* WORKER_H_ */


