/**
 * @file
 *
 * This file contains the implementations of the functions from bsp.h for the
 * STM32F072 Discovery Kit board.
 */

#include <stddef.h> // For NULL.

#include <libopencm3/cm3/assert.h> // For the cm3_assert macros.
#include <libopencm3/stm32/rcc.h> // For the RCC manipulation functions.
#include <libopencm3/stm32/gpio.h> // For the GPIO manipulation functions.
#include <libopencm3/stm32/usart.h> // For UART.
#include <libopencm3/stm32/timer.h> // For the timer functions.
#include <libopencm3/stm32/dma.h> // For the DMA controller.
#include <libopencm3/cm3/nvic.h> // For nvic* functions.

#include <mouros/common.h> // For ARRAY_SIZE()

#include <libopencm3/cm3/cortex.h> // For the critical section macros.

#include "../bsp.h" // For the BSP declarations.

static bool is_initialized = false;

static uint16_t led_pin_lut[] = {
	GPIO6, /** Red */
	GPIO7, /** Blue */
	GPIO8, /** Orange */
	GPIO9 /** Green */
};

mailbox_t bsp_rx_buffer;
static char rx_buffer_mem[BSP_RX_BUFFER_SIZE];

mailbox_t bsp_tx_buffer;
static char tx_buffer_mem[BSP_TX_BUFFER_SIZE];


static void enable_usart1_tx_interrupt(void)
{
	usart_enable_tx_interrupt(USART1);
}

/**
 * Initializes the clocks and GPIOS for the LEDs to work.
 */
static void led_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
	                led_pin_lut[LED1] | led_pin_lut[LED2] |
	                led_pin_lut[LED3] | led_pin_lut[LED4]);

	gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_LOW,
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
	                    enable_usart1_tx_interrupt);

	rcc_periph_clock_enable(RCC_GPIOA);

	rcc_periph_clock_enable(RCC_USART1);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO9 | GPIO10);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO9 | GPIO10);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_stopbits(USART1, USART_CR2_STOP_1_0BIT);

	usart_enable_rx_interrupt(USART1);

	usart_enable(USART1);

	nvic_set_priority(NVIC_USART1_IRQ, 0);
	nvic_enable_irq(NVIC_USART1_IRQ);
}

void bsp_init(void)
{
	cm3_assert(!is_initialized);

	// Main clock setup
	rcc_clock_setup_in_hsi48_out_48mhz();


	led_init();

	comm_init();

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


/**
 * Interrupt handler for the USART1 peripheral.
 *
 * If the interrupt was caused by a received character, the handler adds the
 * chararacter to the rx_buffer. If the interrupt was caused by the peripheral
 * being ready to send a character, the handler takes one from the tx_buffer,
 * and writes it to the peripheral. In case the interrupt was caused by a USART
 * peripheral buffer overrun (typically happens during debugging), the handler
 * clears that interrupt flag.
 */
void usart1_isr(void)
{
	if (usart_get_flag(USART1, USART_ISR_RXNE)) {
		char ch = (char) usart_recv(USART1);
		os_char_buffer_write_ch(&bsp_rx_buffer, ch);

	} else if (usart_get_flag(USART1, USART_ISR_TXE)) {
		char ch = '\0';
		if (os_char_buffer_read_ch(&bsp_tx_buffer, &ch)) {
			usart_send(USART1, (uint8_t) ch);
		} else {
			usart_disable_tx_interrupt(USART1);
		}

	} else if (usart_get_flag(USART1, USART_ISR_ORE)) {
		USART_ICR(USART1) |= USART_ICR_ORECF;
	}
}
