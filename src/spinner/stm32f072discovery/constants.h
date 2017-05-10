/**
 * @file
 *
 * This file contains compile time settings for the Spinner subsystem of the ratfist-stm32 project, specific to the
 * STM32F072 Discovery board.
 */

#ifndef SPINNER_STM32F072_DISCOVERY_CONSTANTS_H_
#define SPINNER_STM32F072_DISCOVERY_CONSTANTS_H_

/** The maximum number of legs any one spin plan may contain. */
#define MAX_SPIN_PLAN_LEGS 20

/**
 * The number of message structs that may be allocated (i.e. waiting to be
 * processed) at any given time.
 */
#define MSG_STRUCT_POOL_SIZE 10

/**
 * The number of message data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of messages:
 *  - GET_PLAN
 *  - SET_SPIN_STATE
 *  - GET_SPIN_STATE
 *  - SPIN_STATE_REPLY
 */
#define SMALL_SIZED_MSG_POOL_SIZE 10

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
#define MAX_OUTBOUND_MESSAGES 10

/**
 * The maximum number of normal messages, that can be buffered after receiving at
 * any given time.
 */
#define MAX_INBOUND_MESSAGES 10


#endif /* SPINNER_STM32F072_DISCOVERY_CONSTANTS_H_ */


