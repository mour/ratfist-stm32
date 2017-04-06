/**
 * @file
 *
 * This file contains the Spinner module API.
 */

#ifndef SPINNER_H_
#define SPINNER_H_

#include <stdint.h>

#include "../message_dispatcher.h"
#include "../constants.h"

/*
 * Message types
 */
#define SPINNER_MSG_SET_PLAN 0
#define SPINNER_MSG_GET_PLAN 1
#define SPINNER_MSG_PLAN_REPLY 2
#define SPINNER_MSG_SET_STATE 3
#define SPINNER_MSG_GET_STATE 4
#define SPINNER_MSG_STATE_REPLY 5
#define SPINNER_MSG_RET_VAL 6
#define SPINNER_MSG_NUM_MESSAGE_TYPES 7


/*
 * Spinner states
 */
#define SPINNER_STATE_STOPPED 0
#define SPINNER_STATE_RUNNING 1
#define SPINNER_STATE_SPINNING_DOWN 2
#define SPINNER_STATE_NUM_STATES 3

/*
 * Message payloads
 */
/**
 * Used by SET_PLAN, PLAN_REPLY.
 */
struct spin_plan_data {
	uint8_t channel_num;

	uint32_t plan_leg_count;

	struct pwm_leg {
		uint32_t duration_msecs;
		float target_pct;
	} plan_legs[MAX_SPIN_PLAN_LEGS];
};

/**
 * Used by GET_PLAN, GET_STATE
 */
struct spin_channel {
	uint8_t channel_num;
};

struct ret_val {
	int32_t ret_val;
};

/**
 * Used by SET_STATE
 */
struct spin_state_set_data {
	uint8_t channel_num;
	uint32_t state;
};

/**
 * Used by SPIN_STATE_REPLY
 */
struct spin_state_data {
	uint8_t channel_num;
	uint32_t state;
	uint32_t plan_time_elapsed_msecs;
	float output_val_pct;
};





struct message *spinner_alloc_message(uint32_t msg_type_id);
void spinner_free_message(struct message *msg);



void spinner_init(void);

#endif /* SPINNER_H_ */


