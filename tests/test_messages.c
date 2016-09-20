/**
 * @file
 *
 * This file contains tests for the message factory.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/messages.h"
#include "../src/errors.h"

static bool float_equals(float lhs, float rhs) {
	return fabsf(lhs - rhs) < FLT_EPSILON;
}

static void init_test(void **state)
{
	(void) state;

	// The pool_alloc_t structs & backing mem are static (local) in
	// messages.c. I.e. can't test their values when passed to
	// os_pool_alloc_init.
	expect_any_count(os_pool_alloc_init, alloc, 3);
	expect_any_count(os_pool_alloc_init, backing_mem, 3);

	// Check allocation of message struct pool.
	expect_value(os_pool_alloc_init,
	             block_size,
	             sizeof(struct message));
	expect_value(os_pool_alloc_init,
	             num_blocks,
	             MSG_STRUCT_POOL_SIZE);

	// Check allocation of small sized struct pool.
	expect_any(os_pool_alloc_init, block_size);
	expect_value(os_pool_alloc_init,
	             num_blocks,
	             SMALL_SIZED_MSG_POOL_SIZE);

	// Check allocation of spin_plan_data struct pool.
	expect_value(os_pool_alloc_init,
	             block_size,
	             sizeof(struct spin_plan_data));
	expect_value(os_pool_alloc_init,
	             num_blocks,
	             SPIN_PLAN_DATA_MSG_POOL_SIZE);

	msg_init();
}

static void parse_set_plan_msg_test(void **state)
{
	(void) state;

	struct message test_msg;
	struct spin_plan_data test_data;

	// Parse simple plan.
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	char test_str1[] = "SET_PLAN,0,12,32.3,42,12";
	struct message *msg = msg_parse_message(test_str1);

	assert_non_null(msg);

	assert_int_equal(msg->type, MSG_SET_PLAN);

	struct spin_plan_data *sp_data = msg->data;

	assert_int_equal(sp_data->channel_num, 0);
	assert_int_equal(sp_data->spin_plan_leg_count, 2);

	assert_int_equal(sp_data->plan_legs[0].duration_msecs, 12);
	assert_true(float_equals(sp_data->plan_legs[0].target_pct, 32.3f));

	assert_int_equal(sp_data->plan_legs[1].duration_msecs, 42);
	assert_true(float_equals(sp_data->plan_legs[1].target_pct, 12.0f));


	// Check parsing fails for plans that are too long
	memset(&test_msg, 0, sizeof(test_msg));
	memset(&test_data, 0, sizeof(test_data));

	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str2[MAX_SPIN_PLAN_LEGS * 10]; // Obviously big enough.
	int len = sprintf(test_str2, "SET_PLAN,1");
	for (int i = 0; i < MAX_SPIN_PLAN_LEGS + 1; i++) {
		len += sprintf(&test_str2[len], ",2,2");
	}

	assert_null(msg_parse_message(test_str2));
	assert_int_equal(errno, MALFORMED_MESSAGE_ERROR);


	// Check mem alloc error situations
	// Fail when allocating the message struct
	expect_any(os_pool_alloc_take, alloc);
	will_return(os_pool_alloc_take, NULL);

	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);

	// Fail when allocating the plan_data struct
	errno = NO_ERROR;
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, NULL);

	expect_any(os_pool_alloc_give, alloc);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);


	// Try different variations of invalid messages
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str3[] = "SET_PLAN,";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str3));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str4[] = "SET_PLAN,-1,12,32,42,12";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str4));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str5[] = "SET_PLAN,2";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str5));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str6[] = "SET_PLAN,2,12,32,42";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str6));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str7[] = "SET_PLAN,1,12,32a,42,12";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str7));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str8[] = "SET_PLAN,2,12,32,4D2,12l";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str8));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);

}

static void parse_get_plan_msg_test(void **state)
{
	(void) state;

	struct message test_msg;
	struct spin_plan_channel test_data;

	// Parse correct request
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	char test_str1[] = "GET_PLAN,0";
	struct message *msg = msg_parse_message(test_str1);

	assert_non_null(msg);

	assert_int_equal(msg->type, MSG_GET_PLAN);

	struct spin_plan_channel *sp_data = msg->data;
	assert_int_equal(sp_data->channel_num, 0);

	// Try with missing channel number
	memset(&test_msg, 0, sizeof(test_msg));
	memset(&test_data, 0, sizeof(test_data));

	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str2[] = "GET_PLAN";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str2));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	// Try with invalid channel number
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str3[] = "GET_PLAN,12345";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str3));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	// Try message with extra fields
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str4[] = "GET_PLAN,6,abcd";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str4));
	assert_int_equal(errno, MALFORMED_MESSAGE_ERROR);


	// Try mem alloc failures
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, NULL);

	expect_any(os_pool_alloc_give, alloc);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);


	expect_any(os_pool_alloc_take, alloc);
	will_return(os_pool_alloc_take, NULL);

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);
}

static void parse_set_spin_state_test(void **state)
{
	(void) state;

	struct message test_msg;
	struct spin_state_set_data test_data;

	// Parse correct on request
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	char test_str1[] = "SET_SPIN_STATE,1,ON";
	struct message *msg = msg_parse_message(test_str1);

	assert_non_null(msg);
	assert_int_equal(msg->type, MSG_SET_SPIN_STATE);

	struct spin_state_set_data *sp_data = msg->data;
	assert_int_equal(sp_data->channel_num, 1);
	assert_int_equal(sp_data->state, SPIN_RUNNING);


	// Parse correct off request
	memset(&test_msg, 0, sizeof(test_msg));
	memset(&test_data, 0, sizeof(test_data));

	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	char test_str2[] = "SET_SPIN_STATE,3,OFF";

	assert_non_null(msg_parse_message(test_str2));
	assert_int_equal(msg->type, MSG_SET_SPIN_STATE);

	sp_data = msg->data;
	assert_int_equal(sp_data->channel_num, 3);
	assert_int_equal(sp_data->state, SPIN_STOPPED);


	// Check with invalid channel number
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str3[] = "SET_SPIN_STATE,3a,OFF";

	errno = NO_ERROR;

	assert_null(msg_parse_message(test_str3));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	// Check invalid channel state
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str4[] = "SET_SPIN_STATE,6,SPINDOWN";

	errno = NO_ERROR;

	assert_null(msg_parse_message(test_str4));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	// Check with missing & extra fields
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str5[] = "SET_SPIN_STATE,";

	errno = NO_ERROR;

	assert_null(msg_parse_message(test_str5));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str6[] = "SET_SPIN_STATE,10";

	errno = NO_ERROR;

	assert_null(msg_parse_message(test_str6));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str7[] = "SET_SPIN_STATE,6,ON,HELLO";

	errno = NO_ERROR;

	assert_null(msg_parse_message(test_str7));
	assert_int_equal(errno, MALFORMED_MESSAGE_ERROR);


	// Check mem alloc fail situation
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, NULL);

	expect_any(os_pool_alloc_give, alloc);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);


	expect_any(os_pool_alloc_take, alloc);
	will_return(os_pool_alloc_take, NULL);

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);
}

static void parse_get_spin_state_test(void **state)
{
	(void) state;

	struct message test_msg;
	struct spin_plan_channel test_data;

	// Parse correct request
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	char test_str1[] = "GET_SPIN_STATE,1";
	struct message *msg = msg_parse_message(test_str1);

	assert_non_null(msg);
	assert_int_equal(msg->type, MSG_GET_SPIN_STATE);

	struct spin_plan_channel *sp_data = msg->data;
	assert_int_equal(sp_data->channel_num, 1);


	// Check with invalid channel number
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str2[] = "GET_SPIN_STATE,1a";

	assert_null(msg_parse_message(test_str2));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	// Check with missing channel number
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str3[] = "GET_SPIN_STATE,";

	assert_null(msg_parse_message(test_str3));
	assert_int_equal(errno, MESSAGE_PARSING_ERROR);


	// Check with extra fields
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, &test_data);

	expect_any_count(os_pool_alloc_give, alloc, 2);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_data);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	char test_str4[] = "GET_SPIN_STATE,5,extra";

	assert_null(msg_parse_message(test_str4));
	assert_int_equal(errno, MALFORMED_MESSAGE_ERROR);


	// Check mem alloc fail situation
	expect_any_count(os_pool_alloc_take, alloc, 2);
	will_return(os_pool_alloc_take, &test_msg);
	will_return(os_pool_alloc_take, NULL);

	expect_any(os_pool_alloc_give, alloc);
	expect_value(os_pool_alloc_give, block, (uintptr_t) &test_msg);

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);


	expect_any(os_pool_alloc_take, alloc);
	will_return(os_pool_alloc_take, NULL);

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, MEM_ALLOC_ERROR);
}

static void serialize_spin_plan_reply_test(void **state)
{
	(void) state;

	// Try serializing a valid message
	struct message test_msg;
	struct spin_plan_data test_data = {
		.channel_num = 3,
		.spin_plan_leg_count = 5,
		.plan_legs = {
			{ .duration_msecs = 20, .target_pct = 23.0f },
			{ .duration_msecs = 120, .target_pct = -23.3f },
			{ .duration_msecs = 220, .target_pct = 35.0f },
			{ .duration_msecs = 320, .target_pct = -60.0f },
			{ .duration_msecs = 10, .target_pct = 0.0f }
		}
	};

	test_msg.type = MSG_SPIN_PLAN_REPLY;
	test_msg.data = &test_data;

	char test_buf[100];

	assert_true(msg_serialize_message(&test_msg, test_buf, 100) >= 0);

	char *save_ptr = NULL;
	char *token = strtok_r(test_buf, ",", &save_ptr);
	assert_string_equal(token, "SPIN_PLAN_REPLY");

	// Channel number
	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "3");

	// Plan legs
	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "20");

	token = strtok_r(NULL, ",", &save_ptr);

	char *fp_end_ptr = NULL;
	errno = 0;
	float target_pct = strtof(token, &fp_end_ptr);
	assert_int_equal(errno, 0);
	assert_ptr_not_equal(fp_end_ptr, token);
	assert_int_equal(*fp_end_ptr, '\0');

	assert_true(float_equals(target_pct, 23.0f));


	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "120");

	token = strtok_r(NULL, ",", &save_ptr);

	fp_end_ptr = NULL;
	errno = 0;
	target_pct = strtof(token, &fp_end_ptr);
	assert_int_equal(errno, 0);
	assert_ptr_not_equal(fp_end_ptr, token);
	assert_int_equal(*fp_end_ptr, '\0');

	assert_true(float_equals(target_pct, -23.3f));


	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "220");

	token = strtok_r(NULL, ",", &save_ptr);

	fp_end_ptr = NULL;
	errno = 0;
	target_pct = strtof(token, &fp_end_ptr);
	assert_int_equal(errno, 0);
	assert_ptr_not_equal(fp_end_ptr, token);
	assert_int_equal(*fp_end_ptr, '\0');

	assert_true(float_equals(target_pct, 35.0f));


	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "320");

	token = strtok_r(NULL, ",", &save_ptr);

	fp_end_ptr = NULL;
	errno = 0;
	target_pct = strtof(token, &fp_end_ptr);
	assert_int_equal(errno, 0);
	assert_ptr_not_equal(fp_end_ptr, token);
	assert_int_equal(*fp_end_ptr, '\0');

	assert_true(float_equals(target_pct, -60.0f));


	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "10");

	token = strtok_r(NULL, ",", &save_ptr);

	fp_end_ptr = NULL;
	errno = 0;
	target_pct = strtof(token, &fp_end_ptr);
	assert_int_equal(errno, 0);
	assert_ptr_not_equal(fp_end_ptr, token);
	assert_int_equal(*fp_end_ptr, '\0');

	assert_true(float_equals(target_pct, 0.0f));

	// Make sure that was the last field in the buffer
	assert_null(strtok_r(NULL, ",", &save_ptr));


	// Check buffer to short situations
	errno = 0;
	assert_int_equal(msg_serialize_message(&test_msg, test_buf, 4), -1);
	assert_int_equal(errno, MESSAGE_BUF_TOO_SMALL_ERROR);

	errno = 0;
	assert_int_equal(msg_serialize_message(&test_msg, test_buf, 40), -1);
	assert_int_equal(errno, MESSAGE_BUF_TOO_SMALL_ERROR);
}

static void serialize_spin_state_reply_test(void **state)
{
	(void) state;

	// Try serializing a valid message
	struct message test_msg;
	struct spin_state_data test_data = {
		.channel_num = 3,
		.state = SPIN_SPINNING_DOWN,
		.plan_time_elapsed_msecs = 123456,
		.output_val_pct = 42.1f
	};

	test_msg.type = MSG_SPIN_STATE_REPLY;
	test_msg.data = &test_data;

	char test_buf[100];

	assert_true(msg_serialize_message(&test_msg, test_buf, 100) >= 0);

	char *save_ptr = NULL;
	char *token = strtok_r(test_buf, ",", &save_ptr);
	assert_string_equal(token, "SPIN_STATE_REPLY");

	// Channel number
	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "3");

	// Channel state
	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "SPINNING_DOWN");

	// Plan elapsed time
	token = strtok_r(NULL, ",", &save_ptr);
	assert_string_equal(token, "123456");

	// Output PWM value
	token = strtok_r(NULL, ",", &save_ptr);

	char *fp_end_ptr = NULL;
	errno = 0;
	float target_pct = strtof(token, &fp_end_ptr);
	assert_int_equal(errno, 0);
	assert_ptr_not_equal(fp_end_ptr, token);
	assert_int_equal(*fp_end_ptr, '\0');

	assert_true(float_equals(target_pct, 42.1f));

	// Make sure that was the last field in the buffer
	assert_null(strtok_r(NULL, ",", &save_ptr));


	// Check buffer to short situations
	errno = 0;
	assert_int_equal(msg_serialize_message(&test_msg, test_buf, 4), -1);
	assert_int_equal(errno, MESSAGE_BUF_TOO_SMALL_ERROR);
}

static void unknown_messages_test(void **state)
{
	(void) state;

	// Try to parse an unknown message
	char test_str1[] = "UNKNOWN_MESSAGE,1,23,1";

	errno = NO_ERROR;
	assert_null(msg_parse_message(test_str1));
	assert_int_equal(errno, UNKNOWN_MESSAGE_TYPE_ERROR);



	// Try to serialize an unknown message
	struct message test_msg = {
		.type = 12345,
		.data = NULL
	};

	char output_buf[100];

	expect_assert_failure(msg_serialize_message(&test_msg, output_buf, 100));


	// Try to allocate and deallocate an unknown message
	struct message test_msg2;

	expect_any(os_pool_alloc_take, alloc);
	will_return(os_pool_alloc_take, &test_msg2);

	expect_assert_failure(msg_create_message(1234));

	test_msg2.type = 1234567;
	test_msg2.data = NULL;
	expect_assert_failure(msg_free_message(&test_msg2));
}


int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(init_test),
		cmocka_unit_test(parse_set_plan_msg_test),
		cmocka_unit_test(parse_get_plan_msg_test),
		cmocka_unit_test(parse_set_spin_state_test),
		cmocka_unit_test(parse_get_spin_state_test),
		cmocka_unit_test(serialize_spin_plan_reply_test),
		cmocka_unit_test(serialize_spin_state_reply_test),
		cmocka_unit_test(unknown_messages_test)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
