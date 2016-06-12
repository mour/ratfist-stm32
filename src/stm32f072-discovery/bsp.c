/**
 * @file
 *
 * This file contains the implementations of the functions from bsp.h for the
 * STM32F072 Discovery Kit board.
 */

#include <libopencm3/cm3/assert.h> // For the cm3_assert macros.
#include <libopencm3/stm32/rcc.h> // For the RCC manipulation functions.
#include <libopencm3/stm32/gpio.h> // For the GPIO manipulation functions.

#include "../bsp.h" // For the BSP declarations.

static bool is_initialized = false;

static uint16_t led_pin_lut[] = {
	GPIO6, /** Red */
	GPIO7, /** Blue */
	GPIO8, /** Orange */
	GPIO9 /** Green */
};

void bsp_init(void)
{
	cm3_assert(!is_initialized);

	// Main clock setup
	rcc_clock_setup_in_hsi48_out_48mhz();


	// LED setup
	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
	                led_pin_lut[LED1] | led_pin_lut[LED2] |
	                led_pin_lut[LED3] | led_pin_lut[LED4]);

	gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_LOW,
	                        led_pin_lut[LED1] | led_pin_lut[LED2] |
	                        led_pin_lut[LED3] | led_pin_lut[LED4]);

	is_initialized = true;
}

bool bsp_led_get_state(enum board_led led)
{
	cm3_assert(is_initialized);

	return gpio_get(GPIOC, led_pin_lut[led]);
}

void bsp_led_on(enum board_led led)
{
	cm3_assert(is_initialized);

	gpio_set(GPIOC, led_pin_lut[led]);
}

void bsp_led_off(enum board_led led)
{
	cm3_assert(is_initialized);

	gpio_clear(GPIOC, led_pin_lut[led]);
}

void bsp_led_toggle(enum board_led led)
{
	cm3_assert(is_initialized);

	gpio_toggle(GPIOC, led_pin_lut[led]);
}
