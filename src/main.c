
#include <stddef.h>
#include <stdbool.h>

#include <mouros/common.h>

#include "worker.h"

#include "bsp.h"

#include "message_dispatcher.h"
#include "rust-bindings/spinner.h"

__attribute__((aligned(8)))
uint8_t spinner_comm_task_stack[TASK_STACK_SIZE];

worker_t spinner_comm;

int main(void)
{
	os_init();

	bsp_init();
	msg_init();

	void *spinner_ctx = spinner_get_context();
	worker_task_init(&spinner_comm,
	                 "spinner_comm",
	                 spinner_comm_task_stack,
	                 ARRAY_SIZE(spinner_comm_task_stack),
	                 6,
	                 spinner_comm_loop,
	                 spinner_ctx);

	worker_start(&spinner_comm);

	dispatcher_init();

	os_tasks_start(1000);

	while (true) {
	}
	return 0;
}
