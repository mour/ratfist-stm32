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

enum message_type {
	MSG_SET_PLAN = 0,
	MSG_GET_PLAN,
	MSG_SPIN_PLAN_REPLY,
	MSG_SET_SPIN_STATE,
	MSG_GET_SPIN_STATE,
	MSG_SPIN_STATE_REPLY,
	MSG_NUM_MESSAGE_TYPES
};

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

enum spin_state {
	SPIN_STOPPED = 0, //!< STOPPED
	SPIN_RUNNING,     //!< RUNNING
	SPIN_SPINNING_DOWN,//!< SPINNING_DOWN
	SPIN_NUM_SPIN_STATES
};

/**
 * Used by SET_SPIN_STATE
 */
struct spin_state_set_data {
	uint8_t channel_num;
	enum spin_state state;
};

/**
 * Used by SPIN_SATE_REPLY
 */
struct spin_state_data {
	uint8_t channel_num;
	enum spin_state state;
	uint64_t plan_time_elapsed_msecs;
	float output_val_pct;
};



struct message {
	enum message_type type;
	void *data;
};

void msg_init(void);

struct message *msg_parse_message(char *input_buf);
ssize_t msg_serialize_message(const struct message *msg,
                              char *output_buf,
                              ssize_t output_buf_len);

struct message *msg_create_message(enum message_type type);
void msg_free_message(struct message *msg);

#endif /* MESSAGES_H_ */


