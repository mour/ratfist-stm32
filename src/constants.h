/**
 * @file
 *
 * This file contains compile time settings for the ratfist-stm32 project.
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

/** The size in bytes of the UART RX character buffer. See bsp_rx_buffer. */
#define BSP_RX_BUFFER_SIZE 250

/** The size in bytes of the UART TX character buffer. See bsp_tx_buffer. */
#define BSP_TX_BUFFER_SIZE 250


/**
 * The size in bytes of the maximum size a single message sent over the UART can
 * have. This is including the leading '$' and trailing '\r\n'.
 */
#define BSP_MAX_MESSAGE_LENGTH 250


/** The maximum number of legs any one spin plan may contain. */
#define MAX_SPIN_PLAN_LEGS 10



/**
 * The number of message structs that may be allocated (i.e. waiting to be
 * processed) at any given time.
 */
#define MSG_STRUCT_POOL_SIZE 15

/**
 * The number of message data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of messages:
 *  - GET_PLAN
 *  - SET_SPIN_STATE
 *  - GET_SPIN_STATE
 *  - SPIN_STATE_REPLY
 */
#define SMALL_SIZED_MSG_POOL_SIZE 15

/**
 * The number of message data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of messages:
 *  - SET_PLAN
 *  - SPIN_PLAN_REPLY
 */
#define SPIN_PLAN_DATA_MSG_POOL_SIZE 2

/**
 * The maximum number of error messages that can be scheduled for sending at any
 * given time.
 */
#define MAX_OUTBOUND_ERROR_MESSAGES 10

/**
 * The maximum number of normal messages, that can be scheduled for sending at
 * any given time.
 */
#define MAX_OUTBOUND_MESSAGES 15

/**
 * The stack size of the individual tasks.
 */
#define TASK_STACK_SIZE 4000

/**
 * The MourOS task priority to be used with RX & TX communication tasks.
 */
#define COMM_TASK_PRIORITY 5

/**
 * The number of OS ticks a comm task will sleep for, after emptying its queue,
 * before checking the queue again.
 */
#define COMM_TASK_SLEEP_TIME_TICKS 50

#endif /* CONSTANTS_H_ */


