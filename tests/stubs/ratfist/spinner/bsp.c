/**
 * @file
 *
 * This file contains the stub implementations of the Spinner BSP functions.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../../../src/spinner/bsp.h"

#include <stdbool.h>
#include <stdint.h>

void bsp_spinner_init(void)
{
}

bool bsp_stepper_is_moving(uint8_t stepper_id)
{
	check_expected(stepper_id);

	return mock_type(bool);
}

void bsp_stepper_start(uint8_t stepper_id)
{
	check_expected(stepper_id);
}

void bsp_stepper_stop(uint8_t stepper_id)
{
	check_expected(stepper_id);
}

float bsp_stepper_get_pos(uint8_t stepper_id)
{
	check_expected(stepper_id);

	return mock_type(float);
}

void bsp_stepper_set_stop_pos(uint8_t stepper_id, float stop_pos_deg)
{
	check_expected(stepper_id);
	(void) stop_pos_deg;
}

void bsp_stepper_remove_stop_pos(uint8_t stepper_id)
{
	check_expected(stepper_id);
}

void bsp_stepper_set_rate(uint8_t stepper_id, float rate_pct)
{
	check_expected(stepper_id);
	(void) rate_pct;
}
