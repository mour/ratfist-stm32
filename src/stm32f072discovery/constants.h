/**
 * @file
 *
 * This file contains compile time settings for the ratfist-stm32 project, specific to the STM32F072 Discovery board.
 */

#ifndef STM32F072_DISCOVERY_CONSTANTS_H_
#define STM32F072_DISCOVERY_CONSTANTS_H_

/** The size in bytes of the UART RX character buffer. See bsp_rx_buffer. */
#define BSP_RX_BUFFER_SIZE 250

/** The size in bytes of the UART TX character buffer. See bsp_tx_buffer. */
#define BSP_TX_BUFFER_SIZE 250

/**
 * The size in bytes of the maximum size a single message sent over the UART can
 * have. This is including the leading '$' and trailing '\r\n'.
 */
#define BSP_MAX_MESSAGE_LENGTH 250

/**
 * The stack size of the individual tasks.
 */
#define TASK_STACK_SIZE 2000


#endif /* STM32F072_DISCOVERY_CONSTANTS_H_ */


