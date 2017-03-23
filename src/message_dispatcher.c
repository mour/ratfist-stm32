/**
 * @file
 *
 * TODO Add file description
 */

#include <stddef.h> // For NULL
#include <stdlib.h> // For the string parsing functions.
#include <errno.h> // For errno.
#include <stdio.h> // snprintf
#include <string.h> // For strtok_r

#include <mouros/common.h> // For ARRAY_SIZE

#include "worker.h" // For the workers.
#include "message_dispatcher.h"
#include "bsp.h" // For bsp_rx_buffer & bsp_tx_buffer.
#include "constants.h"
#include "errors.h"



struct rx_worker_context {
	char incoming_msg_buf[BSP_MAX_MESSAGE_LENGTH];
	uint32_t pos_in_buf;
	mailbox_t *rx_char_buffer;
	bool msg_is_incoming;

	struct subsystems *subsystems;

	mailbox_t *err_msg_queue;
};

struct tx_worker_context {
	mailbox_t *tx_char_buffer;

	struct subsystems *subsystems;

	mailbox_t *err_msg_queue;
};

struct subsystems {
	struct subsystem_message_conf *subsystem_configurations[MAX_NUM_COMM_SUBSYSTEMS];
	uint32_t num_subsystems;
};


static void assemble_incoming_message(void *params);
static void check_outgoing_queue(void *params);


static uint8_t calc_checksum(char *buffer, uint32_t len);


static void process_incoming_message(struct rx_worker_context *ctx);

static void process_outgoing_err_message(mailbox_t *tx_char_buffer,
                                         char *subsystem_name,
                                         int32_t err_code);

static void process_outgoing_message(mailbox_t *tx_char_buffer,
                                     struct subsystem_message_conf *conf,
                                     struct message *msg,
                                     mailbox_t *err_msg_queue);

static bool schedule_err_message(mailbox_t *err_msg_queue, int32_t err_code);


static int32_t disp_err_msg_queue_buf[MAX_OUTBOUND_ERROR_MESSAGES];
static mailbox_t disp_err_msg_queue;

static uint8_t rx_worker_stack[TASK_STACK_SIZE];
static worker_t rx_worker;
static struct rx_worker_context rx_context;

static uint8_t tx_worker_stack[TASK_STACK_SIZE];
static worker_t tx_worker;
static struct tx_worker_context tx_context;

static struct subsystems subsystems = {
	.subsystem_configurations = {NULL},
	.num_subsystems = 0
};





static void assemble_incoming_message(void *params)
{
	struct rx_worker_context *context = params;
	uint32_t *pos_in_buf = &(context->pos_in_buf);
	char *incoming_msg_buf = context->incoming_msg_buf;
	mailbox_t *rx_char_buffer = context->rx_char_buffer;
	bool *msg_is_incoming = &(context->msg_is_incoming);

	char ch = '\0';
	bool have_char = os_char_buffer_read_ch(rx_char_buffer, &ch);

	if (have_char) {
		if (ch == '$') {
			*pos_in_buf = 0;
			context->msg_is_incoming = true;
			return;
		}

		if (!context->msg_is_incoming) {
			return;
		}

		incoming_msg_buf[(*pos_in_buf)++] = ch;

		// Smallest valid message would be
		// "$<char>,<char>,<char>*<2byte checksum>\r\n" == 11 bytes
		// We don't save the $ though, so it's 10.
		if (*pos_in_buf >= 10 &&
		    incoming_msg_buf[*pos_in_buf - 2] == '\r' &&
		    incoming_msg_buf[*pos_in_buf - 1] == '\n') {

			uint32_t len = *pos_in_buf;

			*pos_in_buf = 0;
			*msg_is_incoming = false;

			// Validate checksum
			if (incoming_msg_buf[len - 5] != '*') {
				schedule_err_message(context->err_msg_queue, RX_CHECKSUM_ERROR);
				return;
			}

			char *end_ptr = NULL;

			incoming_msg_buf[len - 2] = '\0';
			unsigned long msg_csum = strtoul(&incoming_msg_buf[len - 4], &end_ptr, 16);
			if (end_ptr != &incoming_msg_buf[len - 2] ||
			    msg_csum != calc_checksum(incoming_msg_buf, len - 5)) {

				schedule_err_message(context->err_msg_queue, RX_CHECKSUM_ERROR);
				return;
			}

			// Strip the trailing csum & newline *<2byte checksum>\r\n
			incoming_msg_buf[len - 5] = '\0';

			process_incoming_message(context);
		}

		// Message too long? Can't really recover from that, so just drop this message, and wait for the next
		// one.
		if (*pos_in_buf >= BSP_MAX_MESSAGE_LENGTH) {
			schedule_err_message(context->err_msg_queue, INCOMING_MESSAGE_TOO_LONG_ERROR);
			*pos_in_buf = 0;
			*msg_is_incoming = false;
		}
	} else {
		os_task_sleep(COMM_TASK_SLEEP_TIME_TICKS);
	}
}

static void check_outgoing_queue(void *params)
{
	struct tx_worker_context *context = params;

	// Send out own error messages
	int32_t err_code = NO_ERROR;
	if (os_mailbox_read_atomic(context->err_msg_queue, &err_code)) {
		process_outgoing_err_message(context->tx_char_buffer, "DISPATCHER", err_code);
		return;
	}

	// If no own errors, then send out subsystem errors
	for (uint32_t i = 0; i < context->subsystems->num_subsystems; i++) {
		struct subsystem_message_conf *conf = context->subsystems->subsystem_configurations[i];

		// Subsystems aren't required to have error message queues.
		if (conf->outgoing_err_queue == NULL) {
			continue;
		}

		if (os_mailbox_read_atomic(conf->outgoing_err_queue, &err_code)) {
			process_outgoing_err_message(context->tx_char_buffer, conf->subsystem_name, err_code);
			return;
		}
	}

	// If no subsystem errors, send out regular messages
	for (uint32_t i = 0; i < context->subsystems->num_subsystems; i++) {
		struct subsystem_message_conf *conf = context->subsystems->subsystem_configurations[i];

		// Subsystems aren't required to send messages.
		if (conf->outgoing_msg_queue == NULL) {
			continue;
		}

		struct message *msg = NULL;
		if (os_mailbox_read_atomic(conf->outgoing_msg_queue, &msg)) {
			process_outgoing_message(context->tx_char_buffer, conf, msg, context->err_msg_queue);
			return;
		}
	}

	os_task_sleep(COMM_TASK_SLEEP_TIME_TICKS);
}



static uint8_t calc_checksum(char *buffer, uint32_t len)
{
	uint8_t checksum = 0;
	for (uint32_t i = 0; i < len; i++) {
		checksum ^= (uint8_t) buffer[i];
	}

	return checksum;
}

static void process_incoming_message(struct rx_worker_context *ctx)
{
	char *save_ptr = NULL;
	char *transaction_id_str = strtok_r(ctx->incoming_msg_buf, ",", &save_ptr);

	// Parse the transaction id
	char *end_ptr = NULL;
	errno = NO_ERROR;
	uint32_t transaction_id = strtoul(transaction_id_str, &end_ptr, 10);
	if (errno != NO_ERROR ||  end_ptr == transaction_id_str || *end_ptr != '\0') {

		schedule_err_message(ctx->err_msg_queue, MESSAGE_PARSING_ERROR);
		return;
	}

	// Find the correct subsystem & message handler
	char *subsystem_name = strtok_r(NULL, ",", &save_ptr);
	char *msg_name = strtok_r(NULL, ",", &save_ptr);

	if (subsystem_name == NULL || msg_name == NULL) {
		schedule_err_message(ctx->err_msg_queue, MESSAGE_PARSING_ERROR);
	}

	for (uint32_t i = 0; i < ctx->subsystems->num_subsystems; i++) {
		struct subsystem_message_conf *conf = ctx->subsystems->subsystem_configurations[i];

		if (strcmp(subsystem_name, conf->subsystem_name) == 0) {

			// Find the message handler
			for (uint32_t msg_idx = 0; msg_idx < conf->num_message_types; msg_idx++) {
				if (strcmp(msg_name, conf->message_handlers[msg_idx].message_name) == 0) {

					// Check we have a parsing function
					if (conf->message_handlers[msg_idx].parsing_func == NULL) {
						schedule_err_message(ctx->err_msg_queue, -1); // TODO add err code
						return;
					}

					// Try to allocate a msg struct
					struct message *msg = conf->alloc_message(msg_idx);
					if (msg == NULL) {
						schedule_err_message(ctx->err_msg_queue, -1); // TODO add err code
						return;
					}
					msg->transaction_id = transaction_id;

					// Try to parse the message
					if (!conf->message_handlers[msg_idx].parsing_func(msg, save_ptr)) {
						schedule_err_message(ctx->err_msg_queue, -1); // TODO add err code

						conf->free_message(msg);
						return;
					}

					// Send the message to the subsystem
					if (!os_mailbox_write(conf->incoming_msg_queue, &msg)) {
						schedule_err_message(ctx->err_msg_queue, -2); // TODO add err code

						conf->free_message(msg);
						return;
					}

					// Short circuit return
					return;
				}
			}

			// Found a subsystem, but not a message parsing function.
			schedule_err_message(ctx->err_msg_queue, -3); // TODO add err code
			return;
		}
	}

	// Could not find subsystem
	schedule_err_message(ctx->err_msg_queue, -4); // TODO add err code
}


static void process_outgoing_err_message(mailbox_t *tx_char_buffer,
                                         char *subsystem_name,
                                         int32_t err_code)
{
	char message_buf[BSP_MAX_MESSAGE_LENGTH];
	message_buf[0] = '$';
	uint32_t total_len = 1;

	int payload_len = snprintf(&message_buf[1], ARRAY_SIZE(message_buf) - 1,
	                           "%s,ERROR,%ld", subsystem_name, err_code);
	if (payload_len <= 0) {
		return;
	}

	total_len += (uint32_t) payload_len;
	if (total_len >= ARRAY_SIZE(message_buf)) {
		return;
	}

	uint8_t csum = calc_checksum(&message_buf[1], (uint32_t) payload_len);

	int csum_len = snprintf(&message_buf[total_len],
	                        ARRAY_SIZE(message_buf) - total_len,
	                        "*%02X\r\n", csum);

	if (csum_len <= 0) {
		return;
	}

	total_len += (uint32_t) csum_len;
	if (total_len >= ARRAY_SIZE(message_buf)) {
		return;
	}

	os_char_buffer_write_buf_blocking(tx_char_buffer,
	                                  message_buf,
	                                  total_len);
}


static void process_outgoing_message(mailbox_t *tx_char_buffer,
                                     struct subsystem_message_conf *conf,
                                     struct message *msg,
                                     mailbox_t *err_msg_queue)
{
	char message_buf[BSP_MAX_MESSAGE_LENGTH];
	message_buf[0] = '$';
	uint32_t total_len = 1;

	ssize_t prefix_len = snprintf(&message_buf[1],
	                               ARRAY_SIZE(message_buf) - 1,
	                               "%lu,%s,%s",
	                               msg->transaction_id,
	                               conf->subsystem_name,
	                               conf->message_handlers[msg->type].message_name);

	if (prefix_len <= 0) {
		schedule_err_message(err_msg_queue, MESSAGE_FORMATTING_ERROR);
		goto free_msg;
	}

	ssize_t payload_len = conf->message_handlers[msg->type].serialization_func(
					msg, &message_buf[prefix_len + 1],
					ARRAY_SIZE(message_buf) - (uint32_t) (prefix_len + 1));

	if (payload_len <= 0) {
		schedule_err_message(err_msg_queue, MESSAGE_FORMATTING_ERROR);
		goto free_msg;
	}

	uint8_t csum = calc_checksum(&message_buf[1], (uint32_t) (prefix_len + payload_len));

	total_len += (uint32_t) (prefix_len + payload_len);

	int csum_len = snprintf(&message_buf[total_len],
	                        ARRAY_SIZE(message_buf) - total_len,
	                        "*%02X\r\n", csum);

	if (csum_len <= 0) {
		schedule_err_message(err_msg_queue, MESSAGE_FORMATTING_ERROR);
		goto free_msg;
	}

	total_len += (uint32_t) csum_len;
	if (total_len >= ARRAY_SIZE(message_buf)) {
		schedule_err_message(err_msg_queue, MESSAGE_FORMATTING_ERROR);
		goto free_msg;
	}


	if (os_char_buffer_write_buf(tx_char_buffer,
	                             message_buf,
	                             total_len) != total_len) {
		schedule_err_message(err_msg_queue, TX_BUFFER_FULL);
	}


free_msg:
	conf->free_message(msg);
}


static bool schedule_err_message(mailbox_t *msg_queue, int32_t err_code)
{
	return os_mailbox_write_atomic(msg_queue, &err_code);
}


void dispatcher_init(void)
{
	// Outgoing error message queue
	os_mailbox_init(&disp_err_msg_queue,
	                disp_err_msg_queue_buf,
	                MAX_OUTBOUND_ERROR_MESSAGES,
	                sizeof(int32_t),
	                NULL);


	rx_context.pos_in_buf = 0;
	rx_context.rx_char_buffer = &bsp_rx_buffer;
	rx_context.msg_is_incoming = false;
	rx_context.subsystems = &subsystems;
	rx_context.err_msg_queue = &disp_err_msg_queue;

	worker_task_init(&rx_worker,
	                 "rx_worker",
	                 rx_worker_stack,
	                 TASK_STACK_SIZE,
	                 COMM_TASK_PRIORITY,
	                 assemble_incoming_message,
	                 &rx_context);

	tx_context.tx_char_buffer = &bsp_tx_buffer;
	tx_context.subsystems = &subsystems;
	tx_context.err_msg_queue = &disp_err_msg_queue;

	worker_task_init(&tx_worker,
	                 "tx_worker",
	                 tx_worker_stack,
	                 TASK_STACK_SIZE,
	                 COMM_TASK_PRIORITY,
	                 check_outgoing_queue,
	                 &tx_context);

	// Register the tasks with the scheduler.
	worker_start(&rx_worker);
	worker_start(&tx_worker);
}

void dispatcher_deinit(void)
{
	worker_stop(&rx_worker);
	worker_stop(&tx_worker);

	worker_join(&rx_worker);
	worker_join(&tx_worker);
}

bool dispatcher_register_subsystem(struct subsystem_message_conf *conf)
{
	if (subsystems.num_subsystems >= MAX_NUM_COMM_SUBSYSTEMS) {
		return false;
	}

	subsystems.subsystem_configurations[subsystems.num_subsystems] = conf;
	subsystems.num_subsystems++;

	return true;
}

