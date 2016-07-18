/**
 * @file
 *
 * TODO Add file description.
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include <stdint.h> // For uint32_t
#include <sys/types.h> // For ssize_t

#include "constants.h" // For MAX_SPIN_PLAN_LEGS, ...

#define NO_ERROR (0)
#define MEM_ALLOC_ERROR (-1)
#define PACKET_PARSING_ERROR (-2)
#define MALFORMED_PACKET_ERROR (-3)
#define PACKET_SERIALIZATION_ERROR (-4)
#define UNKNOWN_PACKET_ERROR (-5)

enum event_type {
	EVENT_SET_PLAN = 0,
	EVENT_GET_PLAN,
	EVENT_SPIN_PLAN_REPLY,
	EVENT_SET_SPIN_STATE,
	EVENT_GET_SPIN_STATE,
	EVENT_SPIN_STATE_REPLY,
	EVENT_NUM_EVENT_TYPES
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



struct event {
	enum event_type type;
	void *data;
};

void ev_init(void);

struct event *ev_parse_event(char *input_buf);
ssize_t ev_serialize_event(const struct event *event,
                           char *output_buf,
                           ssize_t output_buf_len);

struct event *ev_create_event(enum event_type type);
void ev_free_event(struct event *event);

#endif /* EVENTS_H_ */


