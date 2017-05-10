/**
 * @file
 *
 * This file contains compile time settings for the Spinner subsystem of the ratfist-stm32 project.
 */

#ifndef SPINNER_CONSTANTS_H_
#define SPINNER_CONSTANTS_H_

// Board specific overrides
#if defined(STM32F072DISCOVERY)
#include "stm32f072discovery/constants.h"
#elif defined(STM32F411DISCOVERY)
#include "stm32f411discovery/constants.h"
#endif

#endif /* SPINNER_CONSTANTS_H_ */


