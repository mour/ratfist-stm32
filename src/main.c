
#include <stdbool.h>

#include <mouros/tasks.h>

#include "bsp.h"
#include "message_dispatcher.h"

#include "spinner/spinner.h"

#include "meteo/meteo.h"

int main(void)
{
	os_init();

	bsp_init();

	dispatcher_init();

#ifdef INCLUDE_SPINNER
	spinner_init();
#endif

#ifdef INCLUDE_METEO
	meteo_init();
#endif

	os_tasks_start(1000);

	while (true) {
	}
	return 0;
}
