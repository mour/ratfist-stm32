/**
 * @file
 *
 * This file contains the implementations of the functions from bsp.h for the
 * STM32F411 Discovery Kit board.
 */

#include <stddef.h> // For NULL.
#include <math.h> // For fabsf()

#include <libopencm3/cm3/assert.h> // For the cm3_assert macros.
#include <libopencm3/stm32/gpio.h> // For the GPIO manipulation functions.
#include <libopencm3/stm32/timer.h> // For the timer functions.
#include <libopencm3/stm32/dma.h> // For the DMA controller.
#include <libopencm3/cm3/nvic.h> // For nvic* functions.

#include <libopencm3/stm32/rcc.h> // For the RCC manipulation functions.
#include <libopencm3/stm32/usart.h> // For UART.
#include <libopencm3/stm32/flash.h>

#include <mouros/common.h> // For ARRAY_SIZE()

#include <libopencm3/cm3/cortex.h> // For the critical section macros.

#include "../bsp.h" // For the BSP declarations.

static bool is_initialized = false;

static uint16_t led_pin_lut[] = {
	GPIO14, /** Red */
	GPIO15, /** Blue */
	GPIO13, /** Orange */
	GPIO12 /** Green */
};

mailbox_t bsp_rx_buffer;
static char rx_buffer_mem[BSP_RX_BUFFER_SIZE];

mailbox_t bsp_tx_buffer;
static char tx_buffer_mem[BSP_TX_BUFFER_SIZE];

static void enable_usart2_tx_interrupt(void) {
	usart_enable_tx_interrupt(USART2);
}

/**
 * Initializes the clocks and GPIOS for the LEDs to work.
 */
static void led_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOD);

	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
	                led_pin_lut[LED1] | led_pin_lut[LED2] |
	                led_pin_lut[LED3] | led_pin_lut[LED4]);

	gpio_set_output_options(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ,
	                        led_pin_lut[LED1] | led_pin_lut[LED2] |
	                        led_pin_lut[LED3] | led_pin_lut[LED4]);
}

/**
 * Initializes the clocks, the UART peripheral, and the RX and TX buffer
 * structures.
 */
static void comm_init(void)
{
	os_char_buffer_init(&bsp_rx_buffer,
	                    rx_buffer_mem,
	                    BSP_RX_BUFFER_SIZE,
	                    NULL);

	os_char_buffer_init(&bsp_tx_buffer,
	                    tx_buffer_mem,
	                    BSP_TX_BUFFER_SIZE,
	                    enable_usart2_tx_interrupt);

	rcc_periph_clock_enable(RCC_GPIOA);

	rcc_periph_clock_enable(RCC_USART2);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO2 | GPIO3);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);

	usart_enable_rx_interrupt(USART2);

	usart_enable(USART2);

	nvic_set_priority(NVIC_USART2_IRQ, 0);
	nvic_enable_irq(NVIC_USART2_IRQ);
}

void bsp_init(void)
{
	cm3_assert(!is_initialized);

	// Main clock setup
	struct rcc_clock_scale clock_config = {
		.pllm = 4,
		.plln = 96,
		.pllp = 2,
		.pllq = 4,
		.pllr = 0,
		.hpre = RCC_CFGR_HPRE_DIV_NONE,
		.ppre1 = RCC_CFGR_PPRE_DIV_2,
		.ppre2 = RCC_CFGR_PPRE_DIV_NONE,
		.power_save = 0,
		.flash_config = FLASH_ACR_LATENCY_3WS | FLASH_ACR_ICE |
		                FLASH_ACR_DCE | FLASH_ACR_PRFTEN,
		.ahb_frequency = 96000000,
		.apb1_frequency = 48000000,
		.apb2_frequency = 96000000
	};

	rcc_clock_setup_hse_3v3(&clock_config);

	led_init();

	comm_init();

	is_initialized = true;
}

bool bsp_led_get_state(enum board_led led)
{
	cm3_assert(is_initialized);

	return gpio_get(GPIOD, led_pin_lut[led]);
	return false;
}

void bsp_led_on(enum board_led led)
{
	cm3_assert(is_initialized);

	gpio_set(GPIOD, led_pin_lut[led]);
}

void bsp_led_off(enum board_led led)
{
	cm3_assert(is_initialized);

	gpio_clear(GPIOD, led_pin_lut[led]);
}

void bsp_led_toggle(enum board_led led)
{
	cm3_assert(is_initialized);

	gpio_toggle(GPIOD, led_pin_lut[led]);
}

/**
 * Interrupt handler for the USART2 peripheral.
 *
 * If the interrupt was caused by a received character, the handler adds the
 * chararacter to the rx_buffer. If the interrupt was caused by the peripheral
 * being ready to send a character, the handler takes one from the tx_buffer,
 * and writes it to the peripheral. In case the interrupt was caused by a USART
 * peripheral buffer overrun (typically happens during debugging), the handler
 * clears that interrupt flag by reading the data register and trying to write
 * it into the rx_buffer.
 */
void usart2_isr(void)
{
	if (usart_get_interrupt_source(USART2, USART_SR_RXNE) ||
	    usart_get_interrupt_source(USART2, USART_SR_ORE)) {
		char ch = (char) usart_recv(USART2);
		os_char_buffer_write_ch(&bsp_rx_buffer, ch);

	} else if (usart_get_interrupt_source(USART2, USART_SR_TXE)) {
		char ch = '\0';
		if (os_char_buffer_read_ch(&bsp_tx_buffer, &ch)) {
			usart_send(USART2, (uint8_t) ch);
		} else {
			usart_disable_tx_interrupt(USART2);
		}
	}
}
