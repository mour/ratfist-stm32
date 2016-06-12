
#include <stddef.h>
#include <stdbool.h>

#include <mouros/tasks.h>

#include "bsp.h"

static void bsp_test_task_func(void *params)
{
	(void) params;

	while (true) {
		bsp_led_on(LED1);
		bsp_led_on(LED2);
		bsp_led_on(LED3);
		bsp_led_on(LED4);


		os_task_sleep(1000);

		bsp_led_off(LED1);
		bsp_led_off(LED2);
		bsp_led_off(LED3);
		bsp_led_off(LED4);

		os_task_sleep(1000);


		for (uint8_t i = 0; i < 10; i++) {
			bsp_led_toggle(LED1);
			bsp_led_toggle(LED2);
			bsp_led_toggle(LED3);
			bsp_led_toggle(LED4);

			os_task_sleep(100);
		}
	}
}


int main(void)
{
	bsp_init();

	os_init();

	task_t bsp_test_task;

	os_task_init_with_stack(&bsp_test_task, "bsp_test_task", 2000, 0,
	                        bsp_test_task_func, NULL);

	os_task_add(&bsp_test_task);

	os_tasks_start(1000);

	while (true) {
	}
	return 0;
}
