/**
 * @file
 *
 * This file contains the declarations of all board-specific function used by
 * Ratfist.
 */

#ifndef BSP_H_
#define BSP_H_

#include <stdbool.h> // For bool...

#include <mouros/char_buffer.h> // For the char buffers

#include "constants.h" // For the TX and RX buffer sizes.


/** The UART RX buffer. Should be manipulated by os_char_buffer_* methods. */
extern mailbox_t bsp_rx_buffer;
/** The UART TX buffer. Should be manipulated by os_char_buffer_* methods. */
extern mailbox_t bsp_tx_buffer;

// TODO Give the LEDs meaningful debug names.
enum board_led {
	LED1 = 0,
	LED2,
	LED3,
	LED4
};

/**
 * Initialization function for the BSP. Initializes the clocks & peripherals.
 *
 * @note Must be called before any other BSP functions.
 *
 * @note Must only be called once.
 */
void bsp_init(void);

/**
 * Returns the state of the LED.
 *
 * @param led The LED in question.
 * @return The status of the LED. True = on, false = off.
 */
bool bsp_led_get_state(enum board_led led);

/**
 * Switches the LED on.
 *
 * @param led The LED to be switched on.
 */
void bsp_led_on(enum board_led led);

/**
 * Switches the LED off.
 *
 * @param led The LED to be switched off.
 */
void bsp_led_off(enum board_led led);

/**
 * Toggles the LED state.
 *
 * @param led The LED to be toggled.
 */
void bsp_led_toggle(enum board_led led);

#endif /* BSP_H_ */
