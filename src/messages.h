/**
 * @file
 *
 * This file contains the struct and function declarations for the ratfist
 * message factory.
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <stdint.h> // For uint32_t
#include <sys/types.h> // For ssize_t

#include "constants.h" // For MAX_SPIN_PLAN_LEGS, ...

#define MSG_SET_PLAN 0
#define MSG_GET_PLAN 1
#define MSG_SPIN_PLAN_REPLY 2
#define MSG_SET_SPIN_STATE 3
#define MSG_GET_SPIN_STATE 4
#define MSG_SPIN_STATE_REPLY 5
#define MSG_RET_VAL 6
#define MSG_NUM_MESSAGE_TYPES 7

/**
 * Used by SET_PLAN, SPIN_PLAN_REPLY.
 */
struct spin_plan_data {
	uint8_t channel_num;

	uint32_t spin_plan_leg_count;

	struct pwm_leg {
		uint32_t duration_msecs;
		float target_pct;
	} plan_legs[MAX_SPIN_PLAN_LEGS];
};

/**
 * Used by GET_PLAN, GET_SPIN_STATE
 */
struct spin_plan_channel {
	uint8_t channel_num;
};

struct ret_val {
	int32_t ret_val;
};

#define SPIN_STOPPED 0
#define SPIN_RUNNING 1
#define SPIN_SPINNING_DOWN 2
#define SPIN_NUM_SPIN_STATES 3

/**
 * Used by SET_SPIN_STATE
 */
struct spin_state_set_data {
	uint8_t channel_num;
	int32_t state;
};

/**
 * Used by SPIN_STATE_REPLY
 */
struct spin_state_data {
	uint8_t channel_num;
	int32_t state;
	uint32_t plan_time_elapsed_msecs;
	float output_val_pct;
};



struct message {
	int32_t type;
	uint32_t transaction_id;
	void *data;
};

void msg_init(void);

struct message *msg_parse_message(char *input_buf);
ssize_t msg_serialize_message(const struct message *msg,
                              char *output_buf,
                              ssize_t output_buf_len);

struct message *msg_create_message(int32_t type);
void msg_free_message(struct message *msg);

#endif /* MESSAGES_H_ */


