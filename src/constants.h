/**
 * @file
 *
 * This file contains compile time settings for the ratfist-stm32 project.
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#ifndef BSP_RX_BUFFER_SIZE
/** The size in bytes of the UART RX character buffer. See bsp_rx_buffer. */
#define BSP_RX_BUFFER_SIZE 1000
#endif

#ifndef BSP_TX_BUFFER_SIZE
/** The size in bytes of the UART TX character buffer. See bsp_tx_buffer. */
#define BSP_TX_BUFFER_SIZE 1000
#endif


#ifndef BSP_MAX_MESSAGE_LENGTH
/**
 * The size in bytes of the maximum size a single message sent over the UART can
 * have. This is including the leading '$' and trailing '\r\n'.
 */
#define BSP_MAX_MESSAGE_LENGTH 1000
#endif


#ifndef MAX_SPIN_PLAN_LEGS
/** The maximum number of legs any one spin plan may contain. */
#define MAX_SPIN_PLAN_LEGS 100
#endif



#ifndef MSG_STRUCT_POOL_SIZE
/**
 * The number of message structs that may be allocated (i.e. waiting to be
 * processed) at any given time.
 */
#define MSG_STRUCT_POOL_SIZE 35
#endif

#ifndef SMALL_SIZED_MSG_POOL_SIZE
/**
 * The number of message data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of messages:
 *  - GET_PLAN
 *  - SET_SPIN_STATE
 *  - GET_SPIN_STATE
 *  - SPIN_STATE_REPLY
 */
#define SMALL_SIZED_MSG_POOL_SIZE 30
#endif

#ifndef SPIN_PLAN_DATA_MSG_POOL_SIZE
/**
 * The number of message data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of messages:
 *  - SET_PLAN
 *  - SPIN_PLAN_REPLY
 */
#define SPIN_PLAN_DATA_MSG_POOL_SIZE 5
#endif

#ifndef MAX_OUTBOUND_ERROR_MESSAGES
/**
 * The maximum number of error messages that can be scheduled for sending at any
 * given time.
 */
#define MAX_OUTBOUND_ERROR_MESSAGES 20
#endif

#ifndef MAX_OUTBOUND_MESSAGES
/**
 * The maximum number of normal messages, that can be scheduled for sending at
 * any given time.
 */
#define MAX_OUTBOUND_MESSAGES 35
#endif

#ifndef TASK_STACK_SIZE
/**
 * The stack size of the individual tasks.
 */
#define TASK_STACK_SIZE 2000
#endif


#ifndef COMM_TASK_PRIORITY
/**
 * The MourOS task priority to be used with RX & TX communication tasks.
 */
#define COMM_TASK_PRIORITY 5
#endif


#endif /* CONSTANTS_H_ */


