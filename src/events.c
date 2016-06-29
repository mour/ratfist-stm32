/**
 * @file
 *
 * TODO Add file description
 */

#include <stddef.h> // For NULL.
#include <string.h> // For strtok_r, strcmp.
#include <stdlib.h> // For strtoul.
#include <errno.h>  // For errno.
#include <limits.h> // For UCHAR_MAX.
#include <stdio.h>  // For snprintf.

#include <libopencm3/cm3/assert.h> // for cm3_assert_failed().

#include <mouros/pool_alloc.h> // For the pool_allocator.

#include "events.h" // Function & struct declarations.



union small_size_event_data {
	struct spin_plan_channel spin_plan_channel;
	struct spin_state_set_data spin_state_set_data;
	struct spin_state_data spin_state_data;
};




static char *event_name_lut[EVENT_NUM_EVENT_TYPES] = {
	"SET_PLAN",
	"GET_PLAN",
	"SPIN_PLAN_REPLY",
	"SET_SPIN_STATE",
	"GET_SPIN_STATE",
	"SPIN_STATE_REPLY"
};

static char *spin_state_lut[SPIN_NUM_SPIN_STATES] = {
	"STOPPED",
	"RUNNING",
	"SPINNING_DOWN"
};

static pool_alloc_t event_pool;
static struct event event_pool_mem[EVENT_STRUCT_POOL_SIZE];

static pool_alloc_t small_sized_event_pool;
static union small_size_event_data small_sized_event_pool_mem[SMALL_SIZED_EVENT_POOL_SIZE];

static pool_alloc_t spin_plan_data_pool;
static struct spin_plan_data spin_plan_data_pool_mem[SPIN_PLAN_DATA_EVENT_POOL_SIZE];


static int parse_set_plan(char *save_ptr, struct event **ev)
{
	struct event *ret_ev = ev_create_event(EVENT_SET_PLAN);
	if (ret_ev == NULL) {
		return -1;
	}

	struct spin_plan_data *data = ret_ev->data;

	int ret_val = 0;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		ret_val = -2;
		goto fail;
	}

	char *end_ptr = NULL;

	// Parse the channel number
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		ret_val = -2;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;

	// Get duration-target pairs.
	for (uint32_t i = 0; i < MAX_SPIN_PLAN_LEGS; i++) {
		// Get duration string.
		token = strtok_r(NULL, ",", &save_ptr);
		if (token == NULL) {
			// Might be at the end of the packet. Valid situation
			// if there were some preceding legs in the plan.
			if (i > 0) {
				data->spin_plan_leg_count = i;
				break;
			} else {
				ret_val = -2;
				goto fail;
			}
		}

		// Parse the duration.
		errno = 0;
		data->plan_legs[i].duration_msecs = strtoul(token, &end_ptr, 10);
		if (errno != 0 ||
		    end_ptr == token || *end_ptr != '\0') {

			ret_val = -2;
			goto fail;
		}


		// Get target string.
		token = strtok_r(NULL, ",", &save_ptr);
		if (token == NULL) {
			ret_val = -2;
			goto fail;
		}

		// Parse the target value.
		errno = 0;
		data->plan_legs[i].target_pct = strtof(token, &end_ptr);
		if (errno != 0 ||
		    end_ptr == token || *end_ptr != '\0') {

			ret_val = -2;
			goto fail;
		}
	}

	// Not at the end of the packet! This means there were too many legs
	// in the plan.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		ret_val = -3;
		goto fail;
	}


	*ev = ret_ev;
	return 0;

fail:
	ev_free_event(ret_ev);
	*ev = NULL;
	return ret_val;
}

static int parse_get_plan(char *save_ptr, struct event **ev)
{
	struct event *ret_ev = ev_create_event(EVENT_GET_PLAN);
	if (ret_ev == NULL) {
		return -1;
	}

	struct spin_plan_channel *data = ret_ev->data;

	int ret_val = 0;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		ret_val = -2;
		goto fail;
	}


	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		ret_val = -2;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;

	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		ret_val = -3;
		goto fail;
	}


	*ev = ret_ev;
	return 0;

fail:
	ev_free_event(ret_ev);
	*ev = NULL;
	return ret_val;
}

static int parse_set_spin_state(char *save_ptr, struct event **ev)
{
	struct event *ret_ev = ev_create_event(EVENT_SET_SPIN_STATE);
	if (ret_ev == NULL) {
		return -1;
	}

	struct spin_state_set_data *data = ret_ev->data;

	int ret_val = 0;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		ret_val = -2;
		goto fail;
	}


	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		ret_val = -2;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;


	// Get the state string.
	token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		ret_val = -2;
		goto fail;
	}

	if (strcmp(token, "ON") == 0) {
		data->state = SPIN_RUNNING;

	} else if (strcmp(token, "OFF") == 0) {
		data->state = SPIN_STOPPED;

	} else {
		ret_val = -2;
		goto fail;
	}


	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		ret_val = -3;
		goto fail;
	}


	*ev = ret_ev;
	return 0;

fail:
	ev_free_event(ret_ev);
	*ev = NULL;
	return ret_val;
}

static int parse_get_spin_state(char *save_ptr, struct event **ev)
{
	struct event *ret_ev = ev_create_event(EVENT_GET_SPIN_STATE);
	if (ret_ev == NULL) {
		return -1;
	}

	struct spin_plan_channel *data = ret_ev->data;

	int ret_val = 0;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		ret_val = -2;
		goto fail;
	}


	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		ret_val = -2;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;

	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		ret_val = -3;
		goto fail;
	}


	*ev = ret_ev;
	return 0;

fail:
	ev_free_event(ret_ev);
	*ev = NULL;
	return ret_val;
}



static ssize_t serialize_spin_plan_reply(const struct event *ev,
                                         char *output_buf,
                                         ssize_t output_buf_len)
{
	struct spin_plan_data *data = ev->data;

	ssize_t len = 0;

	len = snprintf(output_buf, (unsigned long) output_buf_len,
	               "%hhu", data->channel_num);
	if (len >= output_buf_len) {
		return -1;
	}

	for (uint32_t i = 0; i < data->spin_plan_leg_count; i++) {
		len += snprintf(output_buf,
				(unsigned long) (output_buf_len - len),
		                ",%lu,%f",
		                data->plan_legs[i].duration_msecs,
		                data->plan_legs[i].target_pct);

		if (len >= output_buf_len) {
			return -1;
		}
	}

	return len;
}

static ssize_t serialize_spin_state_reply(const struct event *ev,
                                          char *output_buf,
                                          ssize_t output_buf_len)
{
	struct spin_state_data *data = ev->data;

	ssize_t len = 0;

	len = snprintf(output_buf, (unsigned long) output_buf_len,
	               "%hhu,%s,%llu,%f",
	               data->channel_num,
	               spin_state_lut[data->state],
	               data->plan_time_elapsed_msecs,
	               data->output_val_pct);

	if (len >= output_buf_len) {
		return -1;
	}

	return len;
}



void ev_init(void)
{
	os_pool_alloc_init(&event_pool,
	                   event_pool_mem,
	                   sizeof(struct event),
	                   EVENT_STRUCT_POOL_SIZE);

	os_pool_alloc_init(&small_sized_event_pool,
	                   small_sized_event_pool_mem,
	                   sizeof(union small_size_event_data),
	                   SMALL_SIZED_EVENT_POOL_SIZE);

	os_pool_alloc_init(&spin_plan_data_pool,
	                   spin_plan_data_pool_mem,
	                   sizeof(struct spin_plan_data),
	                   SPIN_PLAN_DATA_EVENT_POOL_SIZE);
}


int ev_parse_event(char *input_buf, struct event **event_ptr)
{
	char *save_ptr = NULL;
	char *token = strtok_r(input_buf, ",", &save_ptr);

	struct event *ret_event = NULL;

	int parse_retval = 0;

	if (strcmp(token, event_name_lut[EVENT_SET_PLAN]) == 0) {
		parse_retval = parse_set_plan(save_ptr, &ret_event);

	} else if (strcmp(token, event_name_lut[EVENT_GET_PLAN]) == 0) {
		parse_retval = parse_get_plan(save_ptr, &ret_event);

	} else if (strcmp(token, event_name_lut[EVENT_SET_SPIN_STATE]) == 0) {
		parse_retval = parse_set_spin_state(save_ptr, &ret_event);

	} else if (strcmp(token, event_name_lut[EVENT_GET_SPIN_STATE]) == 0) {
		parse_retval = parse_get_spin_state(save_ptr, &ret_event);

	} else {
		// All other events are outbound. I.e. should not have to know
		// how to serialize them.
		cm3_assert_failed();
	}

	if (parse_retval != 0) {
		cm3_assert_failed();
	} else {
		*event_ptr = ret_event;
		return 0;
	}
}

ssize_t ev_serialize_event(const struct event *event,
                        char *output_buf,
                        ssize_t output_buf_len)
{

	switch (event->type) {
	case EVENT_SPIN_PLAN_REPLY:
		return serialize_spin_plan_reply(event,
		                                 output_buf,
		                                 output_buf_len);
	case EVENT_SPIN_STATE_REPLY:
		return serialize_spin_state_reply(event,
		                                  output_buf,
		                                  output_buf_len);
	default:
		cm3_assert_failed();
	}

	return 0;
}

struct event *ev_create_event(enum event_type type)
{
	struct event *ret = os_pool_alloc_take(&event_pool);
	if (ret == NULL) {
		return NULL;
	}

	void *data = NULL;

	switch (type) {
	case EVENT_SET_PLAN:
	case EVENT_SPIN_PLAN_REPLY:
		data = os_pool_alloc_take(&spin_plan_data_pool);
		break;
	case EVENT_GET_PLAN:
	case EVENT_GET_SPIN_STATE:
	case EVENT_SET_SPIN_STATE:
	case EVENT_SPIN_STATE_REPLY:
		data = os_pool_alloc_take(&small_sized_event_pool);
		break;
	default:
		cm3_assert_failed();
	}

	if (data == NULL) {
		os_pool_alloc_give(&event_pool, ret);
		return NULL;
	}

	ret->type = type;
	ret->data = data;

	return ret;
}

void ev_free_event(struct event *event)
{
	switch (event->type) {
	case EVENT_SET_PLAN:
	case EVENT_SPIN_PLAN_REPLY:
		os_pool_alloc_give(&spin_plan_data_pool, event->data);
		break;
	case EVENT_GET_PLAN:
	case EVENT_GET_SPIN_STATE:
	case EVENT_SET_SPIN_STATE:
	case EVENT_SPIN_STATE_REPLY:
		os_pool_alloc_give(&small_sized_event_pool, event->data);
		break;
	default:
		cm3_assert_failed();
	}


	os_pool_alloc_give(&event_pool, event);
}
