/**
 * @file
 *
 * This file contains the implementation of the initialization of the Spinner
 * module.
 */

#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include <mouros/mailbox.h>
#include <mouros/pool_alloc.h>
#include <mouros/common.h>

#include <libopencm3/cm3/assert.h>

#include "spinner.h"

#include "../worker.h"
#include "../errors.h"
#include "../message_dispatcher.h"
#include "../constants.h"

// Rust init function
void spinner_rust_init(struct subsystem_message_conf *conf);
// Rust entry function
void spinner_comm_loop(void *subsystem_conf);



// Message parsing
static bool parse_set_plan(struct message *msg, char *save_ptr);
static bool parse_get_plan(struct message *msg, char *save_ptr);
static bool parse_set_state(struct message *msg, char *save_ptr);
static bool parse_get_state(struct message *msg, char *save_ptr);


static ssize_t serialize_plan_reply(const struct message *msg,
                                    char *output_buf,
                                    uint32_t output_buf_len);
static ssize_t serialize_state_reply(const struct message *msg,
                                     char *output_buf,
                                     uint32_t output_buf_len);
static ssize_t serialize_ret_val(const struct message *msg,
                                 char *output_buf,
                                 uint32_t output_buf_len);

static struct message_handler msg_handlers[] = {
	{
		.message_name = "SET_PLAN",
		.parsing_func = parse_set_plan,
		.serialization_func = NULL
	},
	{
		.message_name = "GET_PLAN",
		.parsing_func = parse_get_plan,
		.serialization_func = NULL
	},
	{
		.message_name = "PLAN_REPLY",
		.parsing_func = NULL,
		.serialization_func = serialize_plan_reply
	},
	{
		.message_name = "SET_STATE",
		.parsing_func = parse_set_state,
		.serialization_func = NULL
	},
	{
		.message_name = "GET_STATE",
		.parsing_func = parse_get_state,
		.serialization_func = NULL
	},
	{
		.message_name = "STATE_REPLY",
		.parsing_func = NULL,
		.serialization_func = serialize_state_reply
	},
	{
		.message_name = "RET_VAL",
		.parsing_func = NULL,
		.serialization_func = serialize_ret_val
	}
};



static char *spin_state_lut[SPINNER_STATE_NUM_STATES] = {
	"STOPPED",
	"RUNNING",
	"SPINNING_DOWN"
};




static bool parse_set_plan(struct message *msg, char *save_ptr)
{
	struct spin_plan_data *data = msg->data;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		return false;
	}

	char *end_ptr = NULL;

	// Parse the channel number
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 ||
	    end_ptr == token || *end_ptr != '\0' ||
	    ch_num > UCHAR_MAX) {

		return false;
	}

	data->channel_num = (uint8_t) ch_num;

	// Get duration-target pairs.
	for (uint32_t i = 0; i < MAX_SPIN_PLAN_LEGS; i++) {
		// Get duration string.
		token = strtok_r(NULL, ",", &save_ptr);
		if (token == NULL) {
			// End of the packet.
			data->plan_leg_count = i;
			break;
		}

		// Parse the duration.
		errno = 0;
		data->plan_legs[i].duration_msecs = strtoul(token, &end_ptr, 10);
		if (errno != 0 || end_ptr == token || *end_ptr != '\0') {
			return false;
		}

		// Get target string.
		token = strtok_r(NULL, ",", &save_ptr);
		if (token == NULL) {
			return false;
		}

		// Parse the target value.
		errno = 0;
		data->plan_legs[i].target_pct = strtof(token, &end_ptr);
		if (errno != 0 || end_ptr == token || *end_ptr != '\0') {
			return false;
		}
	}

	// Not at the end of the packet! This means there were too many legs
	// in the plan.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		return false;
	}

	return true;
}

static bool parse_get_plan(struct message *msg, char *save_ptr)
{
	struct spin_channel *data = msg->data;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		return false;
	}

	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 || end_ptr == token || *end_ptr != '\0' || ch_num > UCHAR_MAX) {
		return false;
	}

	data->channel_num = (uint8_t) ch_num;

	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		return false;
	}

	return true;
}

static bool parse_set_state(struct message *msg, char *save_ptr)
{
	struct spin_state_set_data *data = msg->data;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		return false;
	}

	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 || end_ptr == token || *end_ptr != '\0' || ch_num > UCHAR_MAX) {
		return false;
	}

	data->channel_num = (uint8_t) ch_num;


	// Get the state string.
	token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		return false;
	}

	if (strcmp(token, "ON") == 0) {
		data->state = SPINNER_STATE_RUNNING;

	} else if (strcmp(token, "OFF") == 0) {
		data->state = SPINNER_STATE_STOPPED;

	} else {
		return false;
	}


	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		return false;
	}

	return true;
}

static bool parse_get_state(struct message *msg, char *save_ptr)
{
	struct spin_channel *data = msg->data;

	// Get channel number string.
	char *token = strtok_r(NULL, ",", &save_ptr);
	if (token == NULL) {
		return false;
	}

	char *end_ptr = NULL;

	// Parse the channel number.
	errno = 0;
	uint32_t ch_num = strtoul(token, &end_ptr, 10);
	if (errno != 0 || end_ptr == token || *end_ptr != '\0' || ch_num > UCHAR_MAX) {
		return false;
	}

	data->channel_num = (uint8_t) ch_num;

	// Not at the end of the packet!! Invalid packet.
	if (strtok_r(NULL, ",", &save_ptr) != NULL) {
		return false;
	}

	return true;
}



static ssize_t serialize_plan_reply(const struct message *msg,
                                    char *output_buf,
                                    uint32_t output_buf_len)
{
	struct spin_plan_data *data = msg->data;

	ssize_t len = 0;
	ssize_t curr_len = snprintf(output_buf, (size_t) output_buf_len,
	                            ",%u", data->channel_num);

	if (curr_len <= 0) {
		return -1;
	}

	len += curr_len;
	if ((uint32_t ) len >= output_buf_len) {
		return -1;
	}

	for (uint32_t i = 0; i < data->plan_leg_count; i++) {
		curr_len = snprintf(&output_buf[len],
		                    (size_t) ((ssize_t) output_buf_len - len),
		                    ",%lu,%f",
		                    data->plan_legs[i].duration_msecs,
		                    data->plan_legs[i].target_pct);

		if (curr_len <= 0) {
			return -1;
		}

		len += curr_len;
		if ((uint32_t ) len >= output_buf_len) {
			return -1;
		}
	}

	return len;
}

static ssize_t serialize_state_reply(const struct message *msg,
                                     char *output_buf,
                                     uint32_t output_buf_len)
{
	struct spin_state_data *data = msg->data;

	ssize_t len = snprintf(output_buf, (size_t) output_buf_len,
	                       ",%u,%s,%lu,%f",
	                       data->channel_num,
	                       spin_state_lut[data->state],
	                       data->plan_time_elapsed_msecs,
	                       data->output_val_pct);

	if (len <= 0 || (uint32_t) len >= output_buf_len) {
		return -1;
	}

	return len;
}

static ssize_t serialize_ret_val(const struct message *msg,
                                 char *output_buf,
                                 uint32_t output_buf_len)
{
	struct ret_val *data = msg->data;

	ssize_t len = snprintf(output_buf, (size_t) output_buf_len,
	                       ",%ld",
	                       data->ret_val);

	if (len <= 0 || (uint32_t) len >= output_buf_len) {
		return -1;
	}

	return len;
}





// Message allocation

union small_size_msg_data {
	struct spin_channel spin_plan_channel;
	struct spin_state_set_data spin_state_set_data;
	struct spin_state_data spin_state_data;
	struct ret_val ret_val;
};


static pool_alloc_t msg_pool;
static struct message msg_pool_mem[MSG_STRUCT_POOL_SIZE];

static pool_alloc_t small_sized_msg_pool;
static union small_size_msg_data small_sized_msg_pool_mem[SMALL_SIZED_MSG_POOL_SIZE];

static pool_alloc_t spin_plan_data_pool;
static struct spin_plan_data spin_plan_data_pool_mem[SPIN_PLAN_DATA_MSG_POOL_SIZE];



struct message *spinner_alloc_message(uint32_t msg_type_id)
{
	struct message *ret = os_pool_alloc_take(&msg_pool);
	if (ret == NULL) {
		return NULL;
	}

	void *data = NULL;

	switch (msg_type_id) {
	case SPINNER_MSG_SET_PLAN:
	case SPINNER_MSG_PLAN_REPLY:
		data = os_pool_alloc_take(&spin_plan_data_pool);
		break;
	case SPINNER_MSG_GET_PLAN:
	case SPINNER_MSG_GET_STATE:
	case SPINNER_MSG_SET_STATE:
	case SPINNER_MSG_STATE_REPLY:
	case SPINNER_MSG_RET_VAL:
		data = os_pool_alloc_take(&small_sized_msg_pool);
		break;
	default:
		cm3_assert_failed();
	}

	if (data == NULL) {
		os_pool_alloc_give(&msg_pool, ret);
		return NULL;
	}

	ret->type = msg_type_id;
	ret->data = data;
	ret->transaction_id = 0;

	return ret;
}

void spinner_free_message(struct message *msg)
{
	switch (msg->type) {
	case SPINNER_MSG_SET_PLAN:
	case SPINNER_MSG_PLAN_REPLY:
		os_pool_alloc_give(&spin_plan_data_pool, msg->data);
		break;
	case SPINNER_MSG_GET_PLAN:
	case SPINNER_MSG_GET_STATE:
	case SPINNER_MSG_SET_STATE:
	case SPINNER_MSG_STATE_REPLY:
	case SPINNER_MSG_RET_VAL:
		os_pool_alloc_give(&small_sized_msg_pool, msg->data);
		break;
	default:
		cm3_assert_failed();
	}


	os_pool_alloc_give(&msg_pool, msg);
}



// Worker
__attribute__((aligned(8)))
static uint8_t spinner_comm_task_stack[TASK_STACK_SIZE];
static worker_t spinner_comm;

// Msg queues
static mailbox_t rx_msg_queue;
static struct message *rx_msg_queue_buf[MAX_INBOUND_MESSAGES];

static mailbox_t tx_msg_queue;
static struct message *tx_msg_queue_buf[MAX_OUTBOUND_MESSAGES];

static mailbox_t tx_err_msg_queue;
static int32_t tx_err_msg_queue_buf[MAX_OUTBOUND_ERROR_MESSAGES];



static struct subsystem_message_conf spinner_conf = {
	.subsystem_name = "SPINNER",
	.message_handlers = msg_handlers,
	.num_message_types = ARRAY_SIZE(msg_handlers),
	.alloc_message = spinner_alloc_message,
	.free_message = spinner_free_message,
	.outgoing_msg_queue = &tx_msg_queue,
	.incoming_msg_queue = &rx_msg_queue,
	.outgoing_err_queue = &tx_err_msg_queue
};


void spinner_init(void)
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


	os_mailbox_init(&rx_msg_queue, rx_msg_queue_buf,
	                ARRAY_SIZE(rx_msg_queue_buf), sizeof(struct message *),
	                NULL);

	os_mailbox_init(&tx_msg_queue, tx_msg_queue_buf,
	                ARRAY_SIZE(tx_msg_queue_buf), sizeof(struct message *),
	                NULL);

	os_mailbox_init(&tx_err_msg_queue, tx_err_msg_queue_buf,
	                ARRAY_SIZE(tx_err_msg_queue_buf), sizeof(int32_t),
	                NULL);


	spinner_rust_init(&spinner_conf);

	dispatcher_register_subsystem(&spinner_conf);

	worker_task_init(&spinner_comm,
	                 "spinner_comm",
	                 spinner_comm_task_stack,
	                 ARRAY_SIZE(spinner_comm_task_stack),
	                 6,
	                 spinner_comm_loop,
	                 NULL);

	worker_start(&spinner_comm);
}
