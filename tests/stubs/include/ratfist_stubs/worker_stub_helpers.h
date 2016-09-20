/**
 * @file
 *
 * TODO Add file description.
 */

#ifndef WORKER_STUB_HELPERS_H_
#define WORKER_STUB_HELPERS_H_

#include <stdint.h>

#include "../../../../src/worker.h"

struct worker_init_data {
	worker_t *worker;
	const char *name;
	uint8_t *stack_base;
	uint32_t stack_size;
	uint8_t priority;
	void (*action)(void *);
	void *action_params;
};

void worker_stubs_init(void);
void worker_stubs_deinit(void);
uint32_t worker_stubs_get_workers(struct worker_init_data **workers);


#endif /* WORKER_STUB_HELPERS_H_ */


