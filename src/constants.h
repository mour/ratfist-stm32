/**
 * @file
 *
 * This file contains compile time settings for the ratfist-stm32 project.
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

/**
 * The MourOS task priority to be used with RX & TX communication tasks.
 */
#define COMM_TASK_PRIORITY 5

/**
 * The number of OS ticks a comm task will sleep for, after emptying its queue,
 * before checking the queue again.
 */
#define COMM_TASK_SLEEP_TIME_TICKS 50

/**
 * The maximum number of subsystems that can be registered with the message
 * dispatcher.
 */
#define MAX_NUM_COMM_SUBSYSTEMS 5

/**
 * The maximum number of error messages that can be scheduled for sending at any
 * given time.
 */
#define MAX_DISPATCHER_ERROR_MESSAGES 10


// Board specific overrides
#if defined(STM32F072DISCOVERY)
#include "stm32f072discovery/constants.h"
#elif defined(STM32F411DISCOVERY)
#include "stm32f411discovery/constants.h"
#endif

#endif /* CONSTANTS_H_ */


