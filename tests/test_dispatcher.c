/**
 * @file
 *
 * This file contains unit tests for the Ratfist message dispatcher.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <errno.h>
#include <string.h>

#include <mouros/char_buffer.h>

#include <ratfist_stubs/worker_stub_helpers.h>
#include <ratfist_stubs/messages_stub_helpers.h>

#include "../src/constants.h"
#include "../src/errors.h"
#include "../src/message_dispatcher.h"

mailbox_t bsp_rx_buffer;
char rx_buffer_data[200];

mailbox_t bsp_tx_buffer;
char tx_buffer_data[200];

mailbox_t spinner_msg_queue;
mailbox_t *SPINNER_MSG_QUEUE_PTR = &spinner_msg_queue;
struct message spinner_msg_queue_buffer[20];

static int setup(void **state)
{
	(void) state;
	worker_stubs_init();

	os_char_buffer_init(&bsp_rx_buffer, rx_buffer_data, 200, NULL);
	os_char_buffer_init(&bsp_tx_buffer, tx_buffer_data, 200, NULL);

	os_mailbox_init(SPINNER_MSG_QUEUE_PTR,
	                spinner_msg_queue_buffer,
	                20, sizeof(struct message),
	                NULL);

	return 0;
}

static int teardown(void **state)
{
	(void) state;
	worker_stubs_deinit();

	return 0;
}

static void init_test(void **state)
{
	(void) state;

	// RX comm task
	expect_any(worker_task_init, worker);
	expect_string(worker_task_init, name, "rx_worker");
	expect_any(worker_task_init, stack_base);
	expect_value(worker_task_init, stack_size, TASK_STACK_SIZE);
	expect_value(worker_task_init, priority, COMM_TASK_PRIORITY);
	expect_any(worker_task_init, action);
	expect_any(worker_task_init, action_params);
	will_return(worker_task_init, true);

	// TX comm task
	expect_any(worker_task_init, worker);
	expect_string(worker_task_init, name, "tx_worker");
	expect_any(worker_task_init, stack_base);
	expect_value(worker_task_init, stack_size, TASK_STACK_SIZE);
	expect_value(worker_task_init, priority, COMM_TASK_PRIORITY);
	expect_any(worker_task_init, action);
	expect_any(worker_task_init, action_params);
	will_return(worker_task_init, true);

	// Task registration
	expect_any(worker_start, worker);
	will_return(worker_start, true);
	expect_any(worker_start, worker);
	will_return(worker_start, true);

	dispatcher_init();
}

static void send_msg_test(void **state)
{
	init_test(state);

	struct worker_init_data *init_data = NULL;
	assert_int_equal(worker_stubs_get_workers(&init_data), 2);

	// Check we have the correct worker task
	assert_string_equal(init_data[1].name, "tx_worker");
	assert_ptr_equal(init_data[1].action_params, &bsp_tx_buffer);

	// Setup a message (as we're using msg stubs, it doesn't need to be
	// valid)
	struct message test_msg = {
		.type = MSG_SPIN_STATE_REPLY,
		.transaction_id = 123,
		.data = NULL
	};


	// Add the message to the outgoing queue.
	dispatcher_send_message(&test_msg);

	// Check tx buffer is empty.
	char buf[200];
	assert_int_equal(os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf)), 0);

	// Setup the serialization function.
	char serialized_msg[] = "SERIALIZED_MESSAGE";
	expect_value(msg_serialize_message, msg, (uintptr_t) &test_msg);
	expect_any(msg_serialize_message, output_buf);
	expect_any(msg_serialize_message, output_buf_len);
	msg_stubs_set_serialization_output(serialized_msg);
	will_return(msg_serialize_message, sizeof(serialized_msg) - 1);

	// Expect the message to be freed.
	expect_value(msg_free_message, msg, (uintptr_t) &test_msg);

	// Do one iteration of the sender thread.
	init_data[1].action(init_data[1].action_params);

	// See what was written to the tx buffer.
	uint32_t len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$123,SERIALIZED_MESSAGE*1A\r\n", buf);

	// The next iteration should not do anything.
	expect_value(os_task_sleep, num_ticks, COMM_TASK_SLEEP_TIME_TICKS);
	init_data[1].action(init_data[1].action_params);
	assert_int_equal(os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf)), 0);


	// Try various failure modes
	expect_value(msg_serialize_message, msg, (uintptr_t) &test_msg);
	expect_any(msg_serialize_message, output_buf);
	expect_any(msg_serialize_message, output_buf_len);
	msg_stubs_set_serialization_output(NULL);
	will_return(msg_serialize_message, -1);
	errno = MESSAGE_FORMATTING_ERROR;

	// Add the message to the outgoing queue.
	dispatcher_send_message(&test_msg);

	// Expect the message to be freed.
	expect_value(msg_free_message, msg, (uintptr_t) &test_msg);

	// Do an iteration. This should fail and should add an error message to
	// the error queue.
	init_data[1].action(init_data[1].action_params);

	// Check that nothing was written to the tx_buffer.
	assert_int_equal(os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf)), 0);

	// Do an iteration. This should send the error message from the last
	// iteration.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	// Check that a MESSAGE_FORMATTING_ERROR was sent.
	assert_string_equal("$ERROR,-9*60\r\n", buf);

	// Check that the tx_buffer is now empty.
	assert_int_equal(os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf)), 0);



	// Try a situation where there's not enough room for the checksum.
	char long_buf[BSP_MAX_MESSAGE_LENGTH - 6];
	memset(long_buf, 'A', BSP_MAX_MESSAGE_LENGTH - 6);

	expect_value(msg_serialize_message, msg, (uintptr_t) &test_msg);
	expect_any(msg_serialize_message, output_buf);
	expect_any(msg_serialize_message, output_buf_len);
	msg_stubs_set_serialization_output(long_buf);
	will_return(msg_serialize_message, BSP_MAX_MESSAGE_LENGTH - 6);
	long_buf[BSP_MAX_MESSAGE_LENGTH - 7] = '\0';

	errno = MESSAGE_FORMATTING_ERROR;

	// Add the message to the outgoing queue.
	dispatcher_send_message(&test_msg);

	// Expect the message to be freed.
	expect_value(msg_free_message, msg, (uintptr_t) &test_msg);

	// Do an iteration. This should fail and should add an error message to
	// the error queue.
	init_data[1].action(init_data[1].action_params);
	assert_int_equal(os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf)), 0);

	// Do an iteration. This should send the error message from the last
	// iteration.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	// Check that a MESSAGE_FORMATTING_ERROR was sent.
	assert_string_equal("$ERROR,-4*6D\r\n", buf);


	// Try a situation where there's not enough room for the checksum.
	memset(long_buf, 'A', BSP_MAX_MESSAGE_LENGTH - 6);

	expect_value(msg_serialize_message, msg, (uintptr_t) &test_msg);
	expect_any(msg_serialize_message, output_buf);
	expect_any(msg_serialize_message, output_buf_len);
	msg_stubs_set_serialization_output(long_buf);
	will_return(msg_serialize_message, BSP_MAX_MESSAGE_LENGTH - 6);
	long_buf[BSP_MAX_MESSAGE_LENGTH - 7] = '\0';
	errno = MESSAGE_FORMATTING_ERROR;

	// Add the message to the outgoing queue.
	dispatcher_send_message(&test_msg);

	// Expect the message to be freed.
	expect_value(msg_free_message, msg, (uintptr_t) &test_msg);

	// Do an iteration. This should fail and should add an error message to
	// the error queue.
	init_data[1].action(init_data[1].action_params);
	assert_int_equal(os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf)), 0);

	// Do an iteration. This should send the error message from the last
	// iteration.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	// Check that a MESSAGE_FORMATTING_ERROR was sent.
	assert_string_equal("$ERROR,-4*6D\r\n", buf);


	// Try to send a message when the TX buffer is full
	while (os_char_buffer_write_ch(&bsp_tx_buffer, 'X'));

	expect_value(msg_serialize_message, msg, (uintptr_t) &test_msg);
	expect_any(msg_serialize_message, output_buf);
	expect_any(msg_serialize_message, output_buf_len);
	msg_stubs_set_serialization_output(serialized_msg);
	will_return(msg_serialize_message, sizeof(serialized_msg) - 1);

	// Add the message to the outgoing queue.
	dispatcher_send_message(&test_msg);

	// Expect the message to be freed.
	expect_value(msg_free_message, msg, (uintptr_t) &test_msg);

	// Do one iteration of the sender thread.
	init_data[1].action(init_data[1].action_params);

	// Empty the full TX buffer
	char ch = '\0';
	while (os_char_buffer_read_ch(&bsp_tx_buffer, &ch)) {
		assert_int_equal(ch, 'X');
	}

	// Do another iteration of the sender thread. This should output an
	// error message about the full TX buffer in the previous iteration.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,-7*6E\r\n", buf);


}

static void recv_msg_test(void **state)
{
	init_test(state);

	struct worker_init_data *init_data = NULL;
	assert_int_equal(worker_stubs_get_workers(&init_data), 2);

	// Check we have the correct worker task
	assert_string_equal(init_data[0].name, "rx_worker");


	// Check that nothing happens if there is no data in the rx_buffer.
	char ch = '\0';
	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	expect_value(os_task_sleep, num_ticks, COMM_TASK_SLEEP_TIME_TICKS);

	init_data[0].action(init_data[0].action_params);

	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	// Check that messages without a $ aren't being parsed
	char no_dollar_str[] = "SET_SPIN_STATE,1,ON*21\r\n";
	assert_int_equal(os_char_buffer_write_str(&bsp_rx_buffer, no_dollar_str),
	                 sizeof(no_dollar_str) - 1);

	for (uint8_t i = 0; i < sizeof(no_dollar_str) - 1; i++) {
		init_data[0].action(init_data[0].action_params);
	}

	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));


	// Try parsing a correctly formatted message
	char set_state_str[] = "$SET_SPIN_STATE,1,ON*21\r\n";
	assert_int_equal(os_char_buffer_write_str(&bsp_rx_buffer, set_state_str),
	                 sizeof(set_state_str) - 1);

	struct spin_state_set_data set_state_msg_data = {
		.channel_num = 1,
		.state = SPIN_RUNNING
	};

	struct message set_state_msg = {
		.type = MSG_SET_SPIN_STATE,
		.data = &set_state_msg_data
	};

	expect_any(msg_parse_message, input_buf);
	will_return(msg_parse_message, &set_state_msg);

	for (uint8_t i = 0; i < sizeof(set_state_str) - 1; i++) {
		init_data[0].action(init_data[0].action_params);
	}

	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));

	expect_value(os_task_sleep, num_ticks, COMM_TASK_SLEEP_TIME_TICKS);

	// Do an iteration of the sender thread and make sure no message was
	// output.
	init_data[1].action(init_data[1].action_params);
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));


	// Try receiving a message with a bad checksum.
	char set_state_bad_csum_str[] = "$SET_SPIN_STATE,1,ON*20\r\n";
	assert_int_equal(os_char_buffer_write_str(&bsp_rx_buffer, set_state_bad_csum_str),
	                 sizeof(set_state_bad_csum_str) - 1);

	for (uint8_t i = 0; i < sizeof(set_state_bad_csum_str) - 1; i++) {
		init_data[0].action(init_data[0].action_params);
	}
	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	// Do an iteration of the sender thread and check that it output an err
	// message.
	init_data[1].action(init_data[1].action_params);

	char buf[200];
	uint32_t len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,-6*6F\r\n", buf);


	// Try receiving a message with a malformed checksum.
	char set_state_bad_csum_str2[] = "$SET_SPIN_STATE,1,ON/21\r\n";
	assert_int_equal(os_char_buffer_write_str(&bsp_rx_buffer, set_state_bad_csum_str2),
	                 sizeof(set_state_bad_csum_str2) - 1);

	for (uint8_t i = 0; i < sizeof(set_state_bad_csum_str2) - 1; i++) {
		init_data[0].action(init_data[0].action_params);
	}
	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	// Do an iteration of the sender thread and check that it output an err
	// message.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,-6*6F\r\n", buf);


	// Try receiving an unknown message.
	char unknown_msg_str[] = "$UNKNOWN_MESSAGE,1234*74\r\n";
	assert_int_equal(os_char_buffer_write_str(&bsp_rx_buffer, unknown_msg_str),
	                 sizeof(unknown_msg_str) - 1);

	expect_any(msg_parse_message, input_buf);
	will_return(msg_parse_message, NULL);
	errno = UNKNOWN_MESSAGE_TYPE_ERROR;

	for (uint8_t i = 0; i < sizeof(unknown_msg_str) - 1; i++) {
		init_data[0].action(init_data[0].action_params);
	}
	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));


	// Do an iteration of the sender thread and check that it output an err
	// message.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,-5*6C\r\n", buf);


	// Try sending a message we know how to parse, but don't know where to
	// route to.
	char unroutable_msg_str[] = "$SPIN_STATE_REPLY,1,RUNNING,1234,-0.123*70\r\n";

	assert_int_equal(os_char_buffer_write_str(&bsp_rx_buffer, unroutable_msg_str),
	                 sizeof(unroutable_msg_str) - 1);

	struct spin_state_data ss_data = {
		.channel_num = 1,
		.state = SPIN_RUNNING,
		.plan_time_elapsed_msecs = 1234,
		.output_val_pct = -0.123f
	};

	struct message unroutable_msg = {
		.type = MSG_SPIN_STATE_REPLY,
		.data = &ss_data
	};

	expect_any(msg_parse_message, input_buf);
	will_return(msg_parse_message, &unroutable_msg);
	errno = NO_ERROR;

	// Check that the message will be freed.
	expect_value(msg_free_message, msg, (uintptr_t) &unroutable_msg);

	for (uint8_t i = 0; i < sizeof(unroutable_msg_str) - 1; i++) {
		init_data[0].action(init_data[0].action_params);
	}
	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));


	// Do an iteration of the sender thread and check that it output an err
	// message.
	init_data[1].action(init_data[1].action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,-10*58\r\n", buf);


	// Try receiving a message that is too long.

	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	// The message has to start with a $
	assert_true(os_char_buffer_write_ch(&bsp_rx_buffer, '$'));
	init_data[0].action(init_data[0].action_params);

	// BSP_MAX_MESSAGE_LENGTH is larger than the char buffer size, so we
	// need to immediately read the chars into the message buffer.
	for (uint32_t i = 1; i <= BSP_MAX_MESSAGE_LENGTH; i++) {
		assert_true(os_char_buffer_write_ch(&bsp_rx_buffer, 'X'));
		init_data[0].action(init_data[0].action_params);
	}

	assert_false(os_char_buffer_read_ch(&bsp_rx_buffer, &ch));
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));


	init_data[1].action(init_data[1].action_params);
	len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,-11*59\r\n", buf);
}


static void err_msg_test(void **state)
{
	init_test(state);

	struct worker_init_data *init_data = NULL;
	assert_int_equal(worker_stubs_get_workers(&init_data), 2);

	// Check we have the correct worker task
	assert_string_equal(init_data[1].name, "tx_worker");
	assert_ptr_equal(init_data[1].action_params, &bsp_tx_buffer);


	// Try sending a regular error message
	dispatcher_send_err(12345);

	init_data[1].action(init_data[1].action_params);

	char buf[200];
	uint32_t len = os_char_buffer_read_buf(&bsp_tx_buffer, buf, sizeof(buf));
	buf[len] = '\0';

	assert_string_equal("$ERROR,12345*45\r\n", buf);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(init_test),
		cmocka_unit_test_setup_teardown(send_msg_test, setup, teardown),
		cmocka_unit_test_setup_teardown(recv_msg_test, setup, teardown),
		cmocka_unit_test_setup_teardown(err_msg_test, setup, teardown)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
