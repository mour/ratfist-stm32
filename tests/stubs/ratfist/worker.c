/**
 * @file
 *
 * This file contains the stub version of Ratfist worker threads.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include <ratfist_stubs/worker_stub_helpers.h>

#include "../../../src/worker.h"


bool save_worker_init_data = false;

struct worker_init_data *workers = NULL;
uint32_t num_workers = 0;
uint32_t num_workers_max = 0;

void worker_stubs_init(void)
{
	assert(!save_worker_init_data);

	save_worker_init_data = true;

	num_workers = 0;
	num_workers_max = 10;
	workers = malloc(num_workers_max * sizeof(struct worker_init_data));
}

void worker_stubs_deinit(void)
{
	assert(save_worker_init_data);

	save_worker_init_data = false;

	num_workers = 0;
	num_workers_max = 0;
	free(workers);
}

uint32_t worker_stubs_get_workers(struct worker_init_data **worker_data)
{
	*worker_data = workers;
	return num_workers;
}



bool worker_task_init(worker_t *worker,
                      const char *name,
                      uint8_t *stack_base,
                      uint32_t stack_size,
                      uint8_t priority,
                      void (*action)(void *),
                      void *action_params)
{
	check_expected_ptr(worker);
	check_expected_ptr(name);
	check_expected_ptr(stack_base);
	check_expected(stack_size);
	check_expected(priority);
	check_expected_ptr(action);
	check_expected_ptr(action_params);

	if (save_worker_init_data) {
		if (num_workers >= num_workers_max) {
			num_workers_max *= 2;
			workers = realloc(workers, num_workers_max);
			assert(workers != NULL);
		}

		workers[num_workers].worker = worker;
		workers[num_workers].name = name;
		workers[num_workers].stack_base = stack_base;
		workers[num_workers].stack_size = priority;
		workers[num_workers].action = action;
		workers[num_workers].action_params = action_params;
		num_workers++;
	}

	return mock_type(bool);
}

bool worker_start(worker_t *worker)
{
	check_expected_ptr(worker);

	return mock_type(bool);
}

void worker_stop(worker_t *worker)
{
	check_expected_ptr(worker);
}

void worker_join(worker_t *worker)
{
	check_expected_ptr(worker);
}
