/**
 * @file
 *
 * TODO Add file description.
 */

#ifndef SPINNER_BSP_H_
#define SPINNER_BSP_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes the spinner subsystem.
 */
void bsp_spinner_init(void);

/**
 * Returns the stepper motor state.
 *
 * @param stepper_id Stepper ID.
 * @return True if the stepper is moving, false if it's not.
 */
bool bsp_stepper_is_moving(uint8_t stepper_id);

/**
 * Start the stepper motor.
 *
 * @param stepper_id Stepper ID.
 */
void bsp_stepper_start(uint8_t stepper_id);

/**
 * Stop the stepper motor.
 *
 * @param stepper_id Stepper ID.
 */
void bsp_stepper_stop(uint8_t stepper_id);

/**
 * Get the position (in degrees) of the stepper motor.
 *
 * @param stepper_id Stepper ID.
 * @return Stepper motor position in degrees.
 */
float bsp_stepper_get_pos(uint8_t stepper_id);

/**
 * Set a position (in degrees) at which the stepper motor should stop.
 *
 * This function won't turn the stepper, that still needs to be configured by
 * bsp_stepper_set_rate() and bsp_stepper_start(), but it will set a position at
 * which the motor will stop turning.
 *
 * @param stepper_id   Stepper ID.
 * @param stop_pos_deg Position (in degrees) at which to stop the motor. Values
 *                     not in [0, 360) are allowed, and are normalized to
 *                     [0, 360).
 */
void bsp_stepper_set_stop_pos(uint8_t stepper_id, float stop_pos_deg);

/**
 * Remove a previously set stop position.
 *
 * The stop position remains set even after the motor has stopped turning as a
 * result of bsp_stepper_set_stop_pos().
 *
 * @param stepper_id Stepper ID.
 */
void bsp_stepper_remove_stop_pos(uint8_t stepper_id);

/**
 * Set the stepper spinning rate.
 *
 * rate_pct is a percentage of a the maximum turn rate the motor is capable of.
 * The sign of rate_pct denotes the direction in which to turn.
 *
 * @param stepper_id Stepper ID.
 * @param rate_pct   The percentage of the maximum turn rate at which to turn.
 *                   Values under -100% and over 100% get normalized to -100%
 *                   and 100% respectively.
 */
void bsp_stepper_set_rate(uint8_t stepper_id, float rate_pct);


#endif /* SRC_SPINNER_BSP_H_ */


