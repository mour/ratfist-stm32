
#include <stddef.h>
#include <stdbool.h>

#include <mouros/tasks.h>
#include "worker.h"

#include "bsp.h"

#include "message_dispatcher.h"

#include <stdio.h>


#include "rust-bindings/spinner.h"


uint8_t rust_worker_stack[2000];

int main(void)
{
	os_init();

	bsp_init();

	dispatcher_init();

	worker_t rust_worker;
	worker_task_init(&rust_worker, "rust_worker", rust_worker_stack, 2000, 10, spinner_main_loop, NULL);
	worker_start(&rust_worker);

	os_tasks_start(1000);

	while (true) {
	}
	return 0;
}
