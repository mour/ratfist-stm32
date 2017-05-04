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
#include <mouros/common.h>

#include <ratfist_stubs/worker_stub_helpers.h>
#include <ratfist_stubs/messages_stub_helpers.h>

#include "../src/constants.h"
#include "../src/errors.h"
#include "../src/message_dispatcher.h"

struct fake_data_struct {};

mailbox_t bsp_rx_buffer;
char rx_buffer_data[BSP_MAX_MESSAGE_LENGTH + 10];

mailbox_t bsp_tx_buffer;
char tx_buffer_data[BSP_MAX_MESSAGE_LENGTH + 10];


mailbox_t outgoing_msg_queue;
struct message *outgoing_msg_queue_buf[10];

mailbox_t incoming_msg_queue;
struct message *incoming_msg_queue_buf[10];

mailbox_t outgoing_err_queue;
int32_t outgoing_err_queue_buf[10];

static char serialized_msg_buf[2 * BSP_MAX_MESSAGE_LENGTH];

static struct message *fake_alloc(uint32_t message_type)
{
	check_expected(message_type);

	return mock_ptr_type(struct message*);
}

static void fake_free(struct message *msg_ptr)
{
	check_expected_ptr(msg_ptr);
}

static bool msg_parsing_func(struct message *msg_ptr, char *save_ptr)
{
	check_expected_ptr(msg_ptr);

	return mock_type(bool);
}

static ssize_t msg_serialization_func(const struct message *msg_ptr,
                                      char *output_str,
                                      uint32_t output_str_max_len)
{
	check_expected_ptr(msg_ptr);

	ssize_t ret_val = mock_type(ssize_t);

	if (ret_val > 0) {
		strncpy(output_str, serialized_msg_buf, output_str_max_len);
	}

	return ret_val;
}

#define FAKE_SER_DES_MESSAGE 0
#define FAKE_SER_ONLY_MESSAGE 1
#define FAKE_DES_ONLY_MESSAGE 2
#define FAKE_NO_SER_DES_MESSAGE 3

struct message_handler handlers[] = {
	{
		.message_name = "SER_DES_MESSAGE",
		.parsing_func = msg_parsing_func,
		.serialization_func = msg_serialization_func,
	},
	{
		.message_name = "SER_ONLY_MESSAGE",
		.parsing_func = NULL,
		.serialization_func = msg_serialization_func,
	},
	{
		.message_name = "DES_ONLY_MESSAGE",
		.parsing_func = msg_parsing_func,
		.serialization_func = NULL,
	},
	{
		.message_name = "NO_SER_DES_MESSAGE",
		.parsing_func = NULL,
		.serialization_func = NULL,
	},
};

struct subsystem_message_conf fake_subsystem = {
	.subsystem_name = "FAKE",
	.outgoing_msg_queue = &outgoing_msg_queue,
	.incoming_msg_queue = &incoming_msg_queue,
	.outgoing_err_queue = &outgoing_err_queue,
	.message_handlers = handlers,
	.num_message_types = ARRAY_SIZE(handlers),
	.alloc_message = fake_alloc,
	.free_message = fake_free
};

struct subsystem_message_conf mini_fake_subsystem = {
	.subsystem_name = "MINI_FAKE",
	.outgoing_msg_queue = NULL,
	.incoming_msg_queue = NULL,
	.outgoing_err_queue = NULL,
	.message_handlers = NULL,
	.num_message_types = 0,
	.alloc_message = NULL,
	.free_message = NULL
};

static struct worker_init_data *get_rx_worker(void)
{
	struct worker_init_data *init_data = NULL;
	assert_int_equal(worker_stubs_get_workers(&init_data), 2);

	return &init_data[0];
}

static struct worker_init_data *get_tx_worker(void)
{
	struct worker_init_data *init_data = NULL;
	assert_int_equal(worker_stubs_get_workers(&init_data), 2);

	return &init_data[1];
}


static int setup(void **state)
{
	(void) state;
	worker_stubs_init();

	os_char_buffer_init(&bsp_rx_buffer, rx_buffer_data, sizeof(rx_buffer_data), NULL);
	os_char_buffer_init(&bsp_tx_buffer, tx_buffer_data, sizeof(tx_buffer_data), NULL);

	os_mailbox_init(&outgoing_msg_queue,
	                outgoing_msg_queue_buf,
	                ARRAY_SIZE(outgoing_msg_queue_buf),
	                sizeof(struct message*),
	                NULL);
	os_mailbox_init(&incoming_msg_queue,
	                incoming_msg_queue_buf,
	                ARRAY_SIZE(incoming_msg_queue_buf),
	                sizeof(struct message*),
	                NULL);
	os_mailbox_init(&outgoing_err_queue,
	                outgoing_err_queue_buf,
	                ARRAY_SIZE(outgoing_err_queue_buf),
	                sizeof(struct message*),
	                NULL);

	expect_any_count(worker_task_init, worker, 2);
	expect_any_count(worker_task_init, name, 2);
	expect_any_count(worker_task_init, stack_base, 2);
	expect_any_count(worker_task_init, stack_size, 2);
	expect_any_count(worker_task_init, priority, 2);
	expect_any_count(worker_task_init, action, 2);
	expect_any_count(worker_task_init, action_params, 2);
	will_return_count(worker_task_init, true, 2);

	expect_any_count(worker_start, worker, 2);
	will_return_count(worker_start, true, 2);

	dispatcher_init();
	assert_true(dispatcher_register_subsystem(&mini_fake_subsystem));
	assert_true(dispatcher_register_subsystem(&fake_subsystem));

	return 0;
}

static int teardown(void **state)
{
	(void) state;
	worker_stubs_deinit();

	expect_any_count(worker_stop, worker, 2);
	expect_any_count(worker_join, worker, 2);

	dispatcher_deinit();

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
	expect_any_count(worker_start, worker, 2);
	will_return_count(worker_start, true, 2);

	dispatcher_init();

	struct subsystem_message_conf conf = {0};

	for (uint8_t i = 0; i < MAX_NUM_COMM_SUBSYSTEMS; i++) {
		assert_true(dispatcher_register_subsystem(&conf));
	}

	assert_false(dispatcher_register_subsystem(&conf));

	expect_any_count(worker_stop, worker, 2);
	expect_any_count(worker_join, worker, 2);

	dispatcher_deinit();
}

static void send_msg_test(void **state)
{
	(void) state;

	struct worker_init_data *tx_worker = get_tx_worker();


	// No msg in queue
	expect_any(os_task_sleep, num_ticks);
	tx_worker->action(tx_worker->action_params);
	assert_int_equal(bsp_tx_buffer.read_pos, bsp_tx_buffer.write_pos);


	struct fake_data_struct fake_data;
	struct message msg = {
		.type = FAKE_SER_DES_MESSAGE,
		.transaction_id = 1235,
		.data = &fake_data
	};

	struct message *msg_p = &msg;

	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	strcpy(serialized_msg_buf, ",PAYLOAD");

	expect_value(msg_serialization_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_serialization_func, strlen(serialized_msg_buf));

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	char check_buf[200];
	uint32_t len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$1235,FAKE,SER_DES_MESSAGE,PAYLOAD*33\r\n");



	// Try TX buffer full situation
	while (os_char_buffer_write_ch(&bsp_tx_buffer, 'a'));

	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	expect_value(msg_serialization_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_serialization_func, strlen(serialized_msg_buf));

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	char ch = '\0';
	while (os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-9*4B\r\n");



	// Test serialization failed scenario
	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	expect_value(msg_serialization_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_serialization_func, -1);

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-3*41\r\n");



	// Test message name too long scenario
	char long_msg_name[BSP_MAX_MESSAGE_LENGTH];
	memset(long_msg_name, 'A', sizeof(long_msg_name) - 1);
	long_msg_name[BSP_MAX_MESSAGE_LENGTH - 1] = '\0';

	// Temporarily switch the message names
	char *orig_msg_name = handlers[msg_p->type].message_name;
	handlers[msg_p->type].message_name = long_msg_name;


	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	// Switch back to original message name (switch back before asserts so we don't influence other tests)
	handlers[msg_p->type].message_name = orig_msg_name;

	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-11*72\r\n");




	// Test message too long scenario
	memset(serialized_msg_buf, 'a', sizeof(serialized_msg_buf));

	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	expect_value(msg_serialization_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_serialization_func, sizeof(serialized_msg_buf));

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-11*72\r\n");



	// Test message payload really long, but still just enough to fit in the buffer scenario
	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	expect_value(msg_serialization_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_serialization_func, BSP_MAX_MESSAGE_LENGTH - 30);

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-11*72\r\n");




	// Test scenario with missing message serialization function
	msg_p->type = FAKE_DES_ONLY_MESSAGE;
	os_mailbox_write(&outgoing_msg_queue, &msg_p);

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	tx_worker->action(tx_worker->action_params);

	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-8*4A\r\n");
}

static void recv_msg_test(void **state)
{
	(void) state;

	struct worker_init_data *rx_worker = get_rx_worker();
	struct worker_init_data *tx_worker = get_tx_worker();


	// No chars
	expect_any(os_task_sleep, num_ticks);
	rx_worker->action(rx_worker->action_params);


	// Bad checksum situation
	char ch = '\0';
	expect_any(os_task_sleep, num_ticks);
	tx_worker->action(tx_worker->action_params);
	assert_false(os_char_buffer_read_ch(&bsp_tx_buffer, &ch));

	char bad_csum_str[] = "124auoe$456,FAKE,SER_DES_MESSAGE,PAYLOAD*1D\r\n";
	os_char_buffer_write_str(&bsp_rx_buffer, bad_csum_str);

	for (uint32_t i = 0; i < sizeof(bad_csum_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	char check_buf[200];
	uint32_t len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-1*43\r\n");


	char bad_csum_str2[] = "$456,FAKE,SER_DES_MESSAGE,PAYLOADX1D\r\n";
	os_char_buffer_write_str(&bsp_rx_buffer, bad_csum_str2);

	for (uint32_t i = 0; i < sizeof(bad_csum_str2) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-1*43\r\n");


	// Message too long situation
	os_char_buffer_write_ch(&bsp_rx_buffer, '$');
	rx_worker->action(rx_worker->action_params);

	for (uint32_t i = 0; i < BSP_MAX_MESSAGE_LENGTH ; i++) {
		os_char_buffer_write_ch(&bsp_rx_buffer, 'a');
		rx_worker->action(rx_worker->action_params);
	}


	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-11*72\r\n");



	// Malformed message error
	char bad_transaction_id_str[] = "$aoue,FAKE,SER_DES_MESSAGE,PAYLOAD*28\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, bad_transaction_id_str);

	for (uint32_t i = 0; i < sizeof(bad_transaction_id_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-2*40\r\n");


	char no_subsystem_field_str[] = "$12345,*1D\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, no_subsystem_field_str);

	for (uint32_t i = 0; i < sizeof(no_subsystem_field_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-2*40\r\n");


	char no_message_type_field_str[] = "$12345,FAKE,*38\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, no_message_type_field_str);

	for (uint32_t i = 0; i < sizeof(no_message_type_field_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-2*40\r\n");



	// Unknown subsystem scenario
	char unknown_subsystem_str[] = "$12345,NONEXISTENT,MSG*2B\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, unknown_subsystem_str);

	for (uint32_t i = 0; i < sizeof(unknown_subsystem_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-6*44\r\n");



	// Unknown message type scenario
	char unknown_msg_type_str[] = "$12345,FAKE,UNKNOWN*70\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, unknown_msg_type_str);

	for (uint32_t i = 0; i < sizeof(unknown_msg_type_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-7*45\r\n");



	// Missing parsing function scenario
	char missing_parsing_func_str[] = "$12345,FAKE,SER_ONLY_MESSAGE*23\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, missing_parsing_func_str);

	for (uint32_t i = 0; i < sizeof(missing_parsing_func_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-8*4A\r\n");



	// Allocation failure scenario
	char correct_msg_str[] = "$456,FAKE,SER_DES_MESSAGE,PAYLOAD*01\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, correct_msg_str);

	expect_value(fake_alloc, message_type, FAKE_SER_DES_MESSAGE);
	will_return(fake_alloc, NULL);

	for (uint32_t i = 0; i < sizeof(correct_msg_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-4*46\r\n");



	// Payload parsing error scenario
	struct message msg = {0};

	os_char_buffer_write_str(&bsp_rx_buffer, correct_msg_str);

	expect_value(fake_alloc, message_type, FAKE_SER_DES_MESSAGE);
	will_return(fake_alloc, &msg);

	expect_value(msg_parsing_func, msg_ptr, (uintptr_t) &msg);
	will_return(msg_parsing_func, false);

	expect_value(fake_free, msg_ptr, (uintptr_t) &msg);

	for (uint32_t i = 0; i < sizeof(correct_msg_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-2*40\r\n");



	// Subsystem RX queue full scenario
	struct message *msg_p = &msg;
	while (os_mailbox_write(&incoming_msg_queue, &msg_p));

	os_char_buffer_write_str(&bsp_rx_buffer, correct_msg_str);

	expect_value(fake_alloc, message_type, FAKE_SER_DES_MESSAGE);
	will_return(fake_alloc, msg_p);

	expect_value(msg_parsing_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_parsing_func, true);

	expect_value(fake_free, msg_ptr, (uintptr_t) msg_p);

	for (uint32_t i = 0; i < sizeof(correct_msg_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$DISPATCHER,ERROR,-5*47\r\n");



	// All OK scenarios
	while (os_mailbox_read(&incoming_msg_queue, &msg_p));

	os_char_buffer_write_str(&bsp_rx_buffer, correct_msg_str);

	expect_value(fake_alloc, message_type, FAKE_SER_DES_MESSAGE);
	will_return(fake_alloc, msg_p);

	expect_value(msg_parsing_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_parsing_func, true);

	for (uint32_t i = 0; i < sizeof(correct_msg_str) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	assert_true(os_mailbox_read(&incoming_msg_queue, &msg_p));

	assert_int_equal(msg_p->type, FAKE_SER_DES_MESSAGE);
	assert_int_equal(msg_p->transaction_id, 456);


	char correct_msg_str2[] = "$853,FAKE,DES_ONLY_MESSAGE,FIELD1,FIELD2*39\r\n";

	os_char_buffer_write_str(&bsp_rx_buffer, correct_msg_str2);

	expect_value(fake_alloc, message_type, FAKE_DES_ONLY_MESSAGE);
	will_return(fake_alloc, msg_p);

	expect_value(msg_parsing_func, msg_ptr, (uintptr_t) msg_p);
	will_return(msg_parsing_func, true);

	for (uint32_t i = 0; i < sizeof(correct_msg_str2) - 1; i++) {
		rx_worker->action(rx_worker->action_params);
	}

	assert_true(os_mailbox_read(&incoming_msg_queue, &msg_p));

	assert_int_equal(msg_p->type, FAKE_DES_ONLY_MESSAGE);
	assert_int_equal(msg_p->transaction_id, 853);
}


static void err_msg_test(void **state)
{
	struct worker_init_data *tx_worker = get_tx_worker();

	int32_t err_code = -12;
	os_mailbox_write(&outgoing_err_queue, &err_code);

	tx_worker->action(tx_worker->action_params);

	char check_buf[200];
	ssize_t len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$FAKE,ERROR,-12*7F\r\n");



	err_code = 1203;
	os_mailbox_write(&outgoing_err_queue, &err_code);

	tx_worker->action(tx_worker->action_params);

	len = os_char_buffer_read_buf(&bsp_tx_buffer, check_buf, sizeof(check_buf));
	assert_int_not_equal(len, 0);
	check_buf[len] = '\0';

	assert_string_equal(check_buf, "$FAKE,ERROR,1203*51\r\n");
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
