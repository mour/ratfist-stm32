
#include "meteo.h"

#include <stdint.h>
#include <stddef.h>

#include "../constants.h"
#include "../worker.h"


void *rust_meteo_init(void);
void rust_meteo_comm_loop(void *params);

__attribute__((aligned(8)))
static uint8_t meteo_task_stack[TASK_STACK_SIZE];
static worker_t meteo_task;

void meteo_init(void)
{
	void *task_ctx = rust_meteo_init();

	worker_task_init(&meteo_task, "meteo",
	                 meteo_task_stack, sizeof(meteo_task_stack),
					 5, rust_meteo_comm_loop, task_ctx);

	worker_start(&meteo_task);
}
