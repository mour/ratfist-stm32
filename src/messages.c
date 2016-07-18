/**
 * @file
 *
 * This file contains the implementation of the ratfist message factory.
 */

#include <stddef.h> // For NULL.
#include <string.h> // For strtok_r, strcmp.
#include <stdlib.h> // For strtoul.
#include <errno.h>  // For errno.
#include <limits.h> // For UCHAR_MAX.
#include <stdio.h>  // For snprintf.

#include <libopencm3/cm3/assert.h> // for cm3_assert_failed().

#include <mouros/pool_alloc.h> // For the pool_allocator.

#include "messages.h" // Function & struct declarations.



union small_size_msg_data {
	struct spin_plan_channel spin_plan_channel;
	struct spin_state_set_data spin_state_set_data;
	struct spin_state_data spin_state_data;
};


static char *msg_name_lut[MSG_NUM_MESSAGE_TYPES] = {
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

static pool_alloc_t msg_pool;
static struct message msg_pool_mem[MSG_STRUCT_POOL_SIZE];

static pool_alloc_t small_sized_msg_pool;
static union small_size_msg_data small_sized_msg_pool_mem[SMALL_SIZED_MSG_POOL_SIZE];

static pool_alloc_t spin_plan_data_pool;
static struct spin_plan_data spin_plan_data_pool_mem[SPIN_PLAN_DATA_MSG_POOL_SIZE];


static struct message *parse_set_plan(char *save_ptr)
{
	struct message *ret_msg = msg_create_message(MSG_SET_PLAN);
	if (ret_msg == NULL) {
		errno = MEM_ALLOC_ERROR;
		return NULL;
	}

	struct spin_plan_data *data = ret_msg->data;

	int err = NO_ERROR;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}

	char *end_ptr = NULL;

	// Parse the channel number
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		err = MESSAGE_PARSING_ERROR;
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
				err = MESSAGE_PARSING_ERROR;
				goto fail;
			}
		}

		// Parse the duration.
		errno = 0;
		data->plan_legs[i].duration_msecs = strtoul(token, &end_ptr, 10);
		if (errno != 0 ||
		    end_ptr == token || *end_ptr != '\0') {

			err = MESSAGE_PARSING_ERROR;
			goto fail;
		}


		// Get target string.
		token = strtok_r(NULL, ",", &save_ptr);
		if (token == NULL) {
			err = MESSAGE_PARSING_ERROR;
			goto fail;
		}

		// Parse the target value.
		errno = 0;
		data->plan_legs[i].target_pct = strtof(token, &end_ptr);
		if (errno != 0 ||
		    end_ptr == token || *end_ptr != '\0') {

			err = MESSAGE_PARSING_ERROR;
			goto fail;
		}
	}

	// Not at the end of the packet! This means there were too many legs
	// in the plan.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		err = MALFORMED_MESSAGE_ERROR;
		goto fail;
	}


	return ret_msg;

fail:
	msg_free_message(ret_msg);
	errno = err;
	return NULL;
}

static struct message *parse_get_plan(char *save_ptr)
{
	struct message *ret_msg = msg_create_message(MSG_GET_PLAN);
	if (ret_msg == NULL) {
		errno = MEM_ALLOC_ERROR;
		return NULL;
	}

	struct spin_plan_channel *data = ret_msg->data;

	int err = NO_ERROR;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}


	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;

	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		err = MALFORMED_MESSAGE_ERROR;
		goto fail;
	}


	return ret_msg;

fail:
	msg_free_message(ret_msg);
	errno = err;
	return NULL;
}

static struct message *parse_set_spin_state(char *save_ptr)
{
	struct message *ret_msg = msg_create_message(MSG_SET_SPIN_STATE);
	if (ret_msg == NULL) {
		errno = MEM_ALLOC_ERROR;
		return NULL;
	}

	struct spin_state_set_data *data = ret_msg->data;

	int err = NO_ERROR;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}


	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;


	// Get the state string.
	token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}

	if (strcmp(token, "ON") == 0) {
		data->state = SPIN_RUNNING;

	} else if (strcmp(token, "OFF") == 0) {
		data->state = SPIN_STOPPED;

	} else {
		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}


	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		err = MALFORMED_MESSAGE_ERROR;
		goto fail;
	}


	return ret_msg;

fail:
	msg_free_message(ret_msg);
	errno = err;
	return NULL;
}

static struct message *parse_get_spin_state(char *save_ptr)
{
	struct message *ret_msg = msg_create_message(MSG_GET_SPIN_STATE);
	if (ret_msg == NULL) {
		errno = MEM_ALLOC_ERROR;
		return NULL;
	}

	struct spin_plan_channel *data = ret_msg->data;

	int err = NO_ERROR;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}


	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		err = MESSAGE_PARSING_ERROR;
		goto fail;
	}

	data->channel_num = (uint8_t) ch_num;

	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		err = MALFORMED_MESSAGE_ERROR;
		goto fail;
	}


	return ret_msg;

fail:
	msg_free_message(ret_msg);
	errno = err;
	return NULL;
}



static ssize_t serialize_spin_plan_reply(const struct message *msg,
                                         char *output_buf,
                                         ssize_t output_buf_len)
{
	struct spin_plan_data *data = msg->data;

	ssize_t len = 0;

	len = snprintf(output_buf, (unsigned long) output_buf_len,
	               "%s,%hhu",
	               msg_name_lut[MSG_SPIN_PLAN_REPLY],
	               data->channel_num);

	if (len >= output_buf_len) {
		errno = MESSAGE_SERIALIZATION_ERROR;
		return -1;
	}

	for (uint32_t i = 0; i < data->spin_plan_leg_count; i++) {
		len += snprintf(&output_buf[len],
				(unsigned long) (output_buf_len - len),
		                ",%lu,%f",
		                data->plan_legs[i].duration_msecs,
		                data->plan_legs[i].target_pct);

		if (len >= output_buf_len) {
			errno = MESSAGE_SERIALIZATION_ERROR;
			return -1;
		}
	}

	return len;
}

static ssize_t serialize_spin_state_reply(const struct message *msg,
                                          char *output_buf,
                                          ssize_t output_buf_len)
{
	struct spin_state_data *data = msg->data;

	ssize_t len = 0;

	len = snprintf(output_buf, (unsigned long) output_buf_len,
	               "%s,%hhu,%s,%llu,%f",
	               msg_name_lut[MSG_SPIN_STATE_REPLY],
	               data->channel_num,
	               spin_state_lut[data->state],
	               data->plan_time_elapsed_msecs,
	               data->output_val_pct);

	if (len >= output_buf_len) {
		errno = MESSAGE_SERIALIZATION_ERROR;
		return -1;
	}

	return len;
}



void msg_init(void)
{
	os_pool_alloc_init(&msg_pool,
	                   msg_pool_mem,
	                   sizeof(struct message),
	                   MSG_STRUCT_POOL_SIZE);

	os_pool_alloc_init(&small_sized_msg_pool,
	                   small_sized_msg_pool_mem,
	                   sizeof(union small_size_msg_data),
	                   SMALL_SIZED_MSG_POOL_SIZE);

	os_pool_alloc_init(&spin_plan_data_pool,
	                   spin_plan_data_pool_mem,
	                   sizeof(struct spin_plan_data),
	                   SPIN_PLAN_DATA_MSG_POOL_SIZE);
}


struct message *msg_parse_message(char *input_buf)
{
	char *save_ptr = NULL;
	char *token = strtok_r(input_buf, ",", &save_ptr);

	struct message *ret_msg = NULL;

	if (strcmp(token, msg_name_lut[MSG_SET_PLAN]) == 0) {
		ret_msg = parse_set_plan(save_ptr);

	} else if (strcmp(token, msg_name_lut[MSG_GET_PLAN]) == 0) {
		ret_msg = parse_get_plan(save_ptr);

	} else if (strcmp(token, msg_name_lut[MSG_SET_SPIN_STATE]) == 0) {
		ret_msg = parse_set_spin_state(save_ptr);

	} else if (strcmp(token, msg_name_lut[MSG_GET_SPIN_STATE]) == 0) {
		ret_msg = parse_get_spin_state(save_ptr);

	} else {
		// All other messages are outbound. I.e. should not have to know
		// how to parse them.
		errno = UNKNOWN_MESSAGE_TYPE_ERROR;
	}

	return ret_msg;
}

ssize_t msg_serialize_message(const struct message *msg,
                           char *output_buf,
                           ssize_t output_buf_len)
{

	switch (msg->type) {
	case MSG_SPIN_PLAN_REPLY:
		return serialize_spin_plan_reply(msg,
		                                 output_buf,
		                                 output_buf_len);
	case MSG_SPIN_STATE_REPLY:
		return serialize_spin_state_reply(msg,
		                                  output_buf,
		                                  output_buf_len);
	default:
		cm3_assert_failed();
	}

	errno = UNKNOWN_MESSAGE_TYPE_ERROR;
	return -1;
}

struct message *msg_create_message(enum message_type type)
{
	struct message *ret = os_pool_alloc_take(&msg_pool);
	if (ret == NULL) {
		return NULL;
	}

	void *data = NULL;

	switch (type) {
	case MSG_SET_PLAN:
	case MSG_SPIN_PLAN_REPLY:
		data = os_pool_alloc_take(&spin_plan_data_pool);
		break;
	case MSG_GET_PLAN:
	case MSG_GET_SPIN_STATE:
	case MSG_SET_SPIN_STATE:
	case MSG_SPIN_STATE_REPLY:
		data = os_pool_alloc_take(&small_sized_msg_pool);
		break;
	default:
		cm3_assert_failed();
	}

	if (data == NULL) {
		os_pool_alloc_give(&msg_pool, ret);
		return NULL;
	}

	ret->type = type;
	ret->data = data;

	return ret;
}

void msg_free_message(struct message *msg)
{
	switch (msg->type) {
	case MSG_SET_PLAN:
	case MSG_SPIN_PLAN_REPLY:
		os_pool_alloc_give(&spin_plan_data_pool, msg->data);
		break;
	case MSG_GET_PLAN:
	case MSG_GET_SPIN_STATE:
	case MSG_SET_SPIN_STATE:
	case MSG_SPIN_STATE_REPLY:
		os_pool_alloc_give(&small_sized_msg_pool, msg->data);
		break;
	default:
		cm3_assert_failed();
	}


	os_pool_alloc_give(&msg_pool, msg);
}
