/**
 * @file
 *
 * TODO Add file description
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <errno.h>

#include <ratfist_stubs/message_dispatcher_stub_helpers.h>

#include "../src/message_dispatcher.h"

#include "../src/spinner/spinner.h"

void spinner_rust_init(struct subsystem_message_conf *conf);
void spinner_comm_loop(void *params);


void spinner_rust_init(struct subsystem_message_conf *conf)
{
	(void) conf;
}

void spinner_comm_loop(void *params)
{
	(void) params;
}

static bool float_equals_imprecise(float lhs, float rhs, float epsilon) {
	return fabsf(lhs - rhs) < epsilon;
}

static bool float_equals(float lhs, float rhs) {
	return float_equals_imprecise(lhs, rhs, FLT_EPSILON);
}

static struct subsystem_message_conf *init(void)
{
	expect_any(dispatcher_register_subsystem, conf);
	will_return(dispatcher_register_subsystem, true);

	expect_any(worker_task_init, worker);
	expect_any(worker_task_init, name);
	expect_any(worker_task_init, stack_base);
	expect_any(worker_task_init, stack_size);
	expect_any(worker_task_init, priority);
	expect_any(worker_task_init, action);
	expect_any(worker_task_init, action_params);
	will_return(worker_task_init, true);

	expect_any(worker_start, worker);
	will_return(worker_start, true);

	spinner_init();

	return dispatcher_stubs_get_last_conf();
}

static struct message_handler *get_message_handler(struct subsystem_message_conf *conf, char *handler_name)
{
	for (uint8_t i = 0; i < conf->num_message_types; i++) {
		if (strcmp(conf->message_handlers[i].message_name, handler_name) == 0) {
			return &conf->message_handlers[i];
		}
	}

	return NULL;
}

static void init_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	assert_string_equal(conf->subsystem_name, "SPINNER");
	assert_int_equal(conf->outgoing_msg_queue->msg_buf_len / conf->outgoing_msg_queue->msg_size,
	                 MAX_OUTBOUND_MESSAGES);
	assert_int_equal(conf->incoming_msg_queue->msg_buf_len / conf->incoming_msg_queue->msg_size,
	                 MAX_INBOUND_MESSAGES);
	assert_int_equal(conf->outgoing_err_queue->msg_buf_len / conf->outgoing_err_queue->msg_size,
	                 MAX_OUTBOUND_ERROR_MESSAGES);
}


static void alloc_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message *msgs[MSG_STRUCT_POOL_SIZE];
	for (uint8_t i = 0; i < SPIN_PLAN_DATA_MSG_POOL_SIZE; i++) {
		msgs[i] = conf->alloc_message(SPINNER_MSG_SET_PLAN);
		assert_non_null(msgs[i]);
	}

	assert_null(conf->alloc_message(SPINNER_MSG_SET_PLAN));

	for (uint8_t i = 0; i < SPIN_PLAN_DATA_MSG_POOL_SIZE; i++) {
		conf->free_message(msgs[i]);
	}

	struct message *msg = conf->alloc_message(SPINNER_MSG_SET_PLAN);
	assert_non_null(msg);
	conf->free_message(msg);



	for (uint8_t i = 0; i < SMALL_SIZED_MSG_POOL_SIZE; i++) {
		msgs[i] = conf->alloc_message(SPINNER_MSG_SET_STATE);
		assert_non_null(msgs[i]);
	}

	assert_null(conf->alloc_message(SPINNER_MSG_SET_STATE));

	for (uint8_t i = 0; i < SMALL_SIZED_MSG_POOL_SIZE; i++) {
		conf->free_message(msgs[i]);
	}

	msg = conf->alloc_message(SPINNER_MSG_SET_STATE);
	assert_non_null(msg);
	conf->free_message(msg);
}


static void set_plan_parsing_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "SET_PLAN");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_SET_PLAN);
	assert_non_null(msg);



	char empty_msg_str[] = "SET_PLAN";
	char *save_ptr = NULL;

	strtok_r(empty_msg_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str1[] = "SET_PLAN,123.4,40,50";
	strtok_r(invalid_ch_num_str1, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));


	char invalid_ch_num_str2[] = "SET_PLAN,abc,40,50";
	strtok_r(invalid_ch_num_str2, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_duration_str[] = "SET_PLAN,1,12,21,ab1,50";
	strtok_r(invalid_duration_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_target_str[] = "SET_PLAN,1,12,21,40,50/";
	strtok_r(invalid_target_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	// SET_PLAN, + 3, + SPIN_PLAN_LEGS (1,1.1,) + 2 + '\0'
	char message_too_long_str[9 + 2 + 6*MAX_SPIN_PLAN_LEGS + 2];
	strcpy(message_too_long_str, "SET_PLAN,3,");
	for (uint32_t i = 0; i < MAX_SPIN_PLAN_LEGS; i++) {
		strcpy(&message_too_long_str[9 + 2 + 6*i], "1,1.1,");
	}
	strcpy(&message_too_long_str[9 + 2 + 6*MAX_SPIN_PLAN_LEGS], "2");

	strtok_r(message_too_long_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char valid_message_str1[] = "SET_PLAN,4,12,13.3,42,52";
	strtok_r(valid_message_str1, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));

	struct spin_plan_data *data = msg->data;
	assert_int_equal(data->channel_num, 4);
	assert_int_equal(data->plan_leg_count, 2);

	assert_int_equal(data->plan_legs[0].duration_msecs, 12);
	assert_true(float_equals(data->plan_legs[0].target_pct, 13.3f));

	assert_int_equal(data->plan_legs[1].duration_msecs, 42);
	assert_true(float_equals(data->plan_legs[1].target_pct, 52));



	char valid_message_str2[] = "SET_PLAN,6,1,1.1,2,2.2,3,3.3,4,4.4,5,5.5";
	strtok_r(valid_message_str2, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));

	assert_int_equal(data->channel_num, 6);
	assert_int_equal(data->plan_leg_count, 5);

	assert_int_equal(data->plan_legs[0].duration_msecs, 1);
	assert_true(float_equals(data->plan_legs[0].target_pct, 1.1f));

	assert_int_equal(data->plan_legs[1].duration_msecs, 2);
	assert_true(float_equals(data->plan_legs[1].target_pct, 2.2f));

	assert_int_equal(data->plan_legs[2].duration_msecs, 3);
	assert_true(float_equals(data->plan_legs[2].target_pct, 3.3f));

	assert_int_equal(data->plan_legs[3].duration_msecs, 4);
	assert_true(float_equals(data->plan_legs[3].target_pct, 4.4f));

	assert_int_equal(data->plan_legs[4].duration_msecs, 5);
	assert_true(float_equals(data->plan_legs[4].target_pct, 5.5f));
}


static void get_plan_parsing_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "GET_PLAN");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_GET_PLAN);
	assert_non_null(msg);



	char empty_msg_str[] = "GET_PLAN";
	char *save_ptr = NULL;

	strtok_r(empty_msg_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str1[] = "GET_PLAN,a";
	strtok_r(invalid_ch_num_str1, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str2[] = "GET_PLAN,-1";
	strtok_r(invalid_ch_num_str2, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char message_too_long_str[] = "GET_PLAN,4,213";
	strtok_r(message_too_long_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char valid_message_str1[] = "GET_PLAN,6";
	strtok_r(valid_message_str1, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));

	struct spin_channel *data = msg->data;
	assert_int_equal(data->channel_num, 6);



	char valid_message_str2[] = "GET_PLAN,123";
	strtok_r(valid_message_str2, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));

	assert_int_equal(data->channel_num, 123);
}


static void plan_reply_serialization_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "PLAN_REPLY");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_PLAN_REPLY);
	assert_non_null(msg);

	// Test empty plan
	struct spin_plan_data *data = msg->data;
	data->channel_num = 123;
	data->plan_leg_count = 0;

	char output_buf[200];

	ssize_t len = handler->serialization_func(msg, output_buf, sizeof(output_buf));
	assert_int_equal(len, sizeof(",123") - 1);

	assert_int_equal(strncmp(",123", output_buf, (size_t) len), 0);


	// Test OK plan
	data->plan_leg_count = 4;
	data->plan_legs[0].duration_msecs = 42;
	data->plan_legs[0].target_pct = 42.1f;
	data->plan_legs[1].duration_msecs = 142;
	data->plan_legs[1].target_pct = -84.2f;
	data->plan_legs[2].duration_msecs = 111;
	data->plan_legs[2].target_pct = 100.0f;
	data->plan_legs[3].duration_msecs = 20;
	data->plan_legs[3].target_pct = 0.0f;

	// Last byte possibly needed for NULL termination.
	len = handler->serialization_func(msg, output_buf, sizeof(output_buf) - 1);
	assert_in_range(len, 0, sizeof(output_buf) - 1);
	output_buf[len] = '\0';

	char *save_ptr = NULL;
	char *token = strtok_r(output_buf, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "123");

	for (uint8_t i = 0; i < data->plan_leg_count; i++) {
		token = strtok_r(NULL, ",", &save_ptr);
		assert_non_null(token);

		char *end_ptr = NULL;
		errno = 0;
		uint32_t dur = strtoul(token, &end_ptr, 10);
		assert_true(errno == 0 || end_ptr != token || *end_ptr == '\0');

		assert_int_equal(dur, data->plan_legs[i].duration_msecs);


		token = strtok_r(NULL, ",", &save_ptr);
		assert_non_null(token);

		errno = 0;
		end_ptr = NULL;
		float tgt = strtof(token, &end_ptr);
		assert_true(errno == 0 || end_ptr != token || *end_ptr == '\0');

		assert_true(float_equals_imprecise(tgt, data->plan_legs[i].target_pct, 0.000001f));
	}

	token = strtok_r(NULL, ",", &save_ptr);
	assert_null(token);


	// Check output_buf too small situations
	len = handler->serialization_func(msg, output_buf, 3);
	assert_int_equal(len, -1);

	len = handler->serialization_func(msg, output_buf, 30);
	assert_int_equal(len, -1);
}


static void set_state_parsing_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "SET_STATE");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_SET_STATE);
	assert_non_null(msg);


	char empty_msg_str[] = "SET_STATE";
	char *save_ptr = NULL;

	strtok_r(empty_msg_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str1[] = "SET_STATE,a,ON";
	strtok_r(invalid_ch_num_str1, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str2[] = "SET_STATE,-1,ON";
	strtok_r(invalid_ch_num_str2, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_state_str[] = "SET_STATE,1,start";
	strtok_r(invalid_state_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char missing_state_str[] = "SET_STATE,1";
	strtok_r(missing_state_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char extra_packet_field_str[] = "SET_STATE,1,ON,extra";
	strtok_r(extra_packet_field_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char state_on_str[] = "SET_STATE,1,ON";
	strtok_r(state_on_str, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));
	struct spin_state_set_data *data = msg->data;
	assert_int_equal(data->channel_num, 1);
	assert_int_equal(data->state, SPINNER_STATE_RUNNING);



	char state_off_str[] = "SET_STATE,12,OFF";
	strtok_r(state_off_str, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));
	assert_int_equal(data->channel_num, 12);
	assert_int_equal(data->state, SPINNER_STATE_STOPPED);
}


static void get_state_parsing_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "GET_STATE");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_GET_STATE);
	assert_non_null(msg);



	char empty_msg_str[] = "GET_STATE";
	char *save_ptr = NULL;

	strtok_r(empty_msg_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str1[] = "GET_STATE,a";
	strtok_r(invalid_ch_num_str1, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char invalid_ch_num_str2[] = "GET_STATE,-1";
	strtok_r(invalid_ch_num_str2, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char message_too_long_str[] = "GET_STATE,4,213";
	strtok_r(message_too_long_str, ",", &save_ptr);

	assert_false(handler->parsing_func(msg, save_ptr));



	char valid_message_str1[] = "GET_STATE,6";
	strtok_r(valid_message_str1, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));

	struct spin_channel *data = msg->data;
	assert_int_equal(data->channel_num, 6);



	char valid_message_str2[] = "GET_STATE,123";
	strtok_r(valid_message_str2, ",", &save_ptr);

	assert_true(handler->parsing_func(msg, save_ptr));

	assert_int_equal(data->channel_num, 123);
}


static void state_reply_serialization_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "STATE_REPLY");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_STATE_REPLY);
	assert_non_null(msg);


	// Test OK state
	struct spin_state_data *data = msg->data;
	data->channel_num = 12;
	data->output_val_pct = 42.1f;
	data->plan_time_elapsed_msecs = 1234;
	data->state = SPINNER_STATE_RUNNING;

	char output_buf[200];

	// Last byte possibly needed for NULL termination.
	ssize_t len = handler->serialization_func(msg, output_buf, sizeof(output_buf) - 1);
	assert_in_range(len, 0, sizeof(output_buf) - 1);
	output_buf[len] = '\0';


	char *save_ptr = NULL;
	char *token = strtok_r(output_buf, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "12");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "RUNNING");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "1234");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);

	errno = 0;
	char *end_ptr = NULL;
	float curr_output = strtof(token, &end_ptr);
	assert_true(errno == 0 || end_ptr != token || *end_ptr == '\0');

	assert_true(float_equals_imprecise(curr_output, data->output_val_pct, 0.000001f));

	assert_null(strtok_r(NULL, ",", &save_ptr));


	// Test stopped state
	data->channel_num = 4;
	data->output_val_pct = 0.0f;
	data->plan_time_elapsed_msecs = 45678;
	data->state = SPINNER_STATE_STOPPED;

	len = handler->serialization_func(msg, output_buf, sizeof(output_buf) - 1);
	assert_in_range(len, 0, sizeof(output_buf) - 1);
	output_buf[len] = '\0';


	save_ptr = NULL;
	token = strtok_r(output_buf, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "4");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "STOPPED");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "45678");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);

	errno = 0;
	end_ptr = NULL;
	curr_output = strtof(token, &end_ptr);
	assert_true(errno == 0 || end_ptr != token || *end_ptr == '\0');

	assert_true(float_equals_imprecise(curr_output, data->output_val_pct, 0.000001f));

	assert_null(strtok_r(NULL, ",", &save_ptr));


	// Test spindown state
	data->channel_num = 40;
	data->output_val_pct = -12.7f;
	data->plan_time_elapsed_msecs = 231;
	data->state = SPINNER_STATE_SPINNING_DOWN;

	len = handler->serialization_func(msg, output_buf, sizeof(output_buf) - 1);
	assert_in_range(len, 0, sizeof(output_buf) - 1);
	output_buf[len] = '\0';


	save_ptr = NULL;
	token = strtok_r(output_buf, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "40");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "SPINNING_DOWN");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);
	assert_string_equal(token, "231");

	token = strtok_r(NULL, ",", &save_ptr);
	assert_non_null(token);

	errno = 0;
	end_ptr = NULL;
	curr_output = strtof(token, &end_ptr);
	assert_true(errno == 0 || end_ptr != token || *end_ptr == '\0');

	assert_true(float_equals_imprecise(curr_output, data->output_val_pct, 0.000001f));

	assert_null(strtok_r(NULL, ",", &save_ptr));


	// Test output buffer too small scenario
	assert_int_equal(handler->serialization_func(msg, output_buf, 10), -1);
}


static void ret_val_serialization_test(void **state)
{
	(void) state;

	struct subsystem_message_conf *conf = init();
	assert_non_null(conf);

	struct message_handler *handler = get_message_handler(conf, "RET_VAL");
	assert_non_null(handler);

	struct message *msg = conf->alloc_message(SPINNER_MSG_RET_VAL);
	assert_non_null(msg);



	struct ret_val *data = msg->data;
	data->ret_val = 1234;

	char output_buf[200];

	// Last byte possibly needed for NULL termination.
	ssize_t len = handler->serialization_func(msg, output_buf, sizeof(output_buf) - 1);
	assert_in_range(len, 0, sizeof(output_buf) - 1);
	output_buf[len] = '\0';

	char *save_ptr = NULL;
	char *token = strtok_r(output_buf, ",", &save_ptr);
	assert_string_equal(token, "1234");

	assert_null(strtok_r(NULL, ",", &save_ptr));



	data->ret_val = -12;

	len = handler->serialization_func(msg, output_buf, sizeof(output_buf) - 1);
	assert_in_range(len, 0, sizeof(output_buf) - 1);
	output_buf[len] = '\0';

	save_ptr = NULL;
	token = strtok_r(output_buf, ",", &save_ptr);
	assert_string_equal(token, "-12");

	assert_null(strtok_r(NULL, ",", &save_ptr));



	assert_int_equal(handler->serialization_func(msg, output_buf, 3), -1);
}


int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(init_test),
		cmocka_unit_test(alloc_test),
		cmocka_unit_test(set_plan_parsing_test),
		cmocka_unit_test(get_plan_parsing_test),
		cmocka_unit_test(plan_reply_serialization_test),
		cmocka_unit_test(set_state_parsing_test),
		cmocka_unit_test(get_state_parsing_test),
		cmocka_unit_test(state_reply_serialization_test),
		cmocka_unit_test(ret_val_serialization_test)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
