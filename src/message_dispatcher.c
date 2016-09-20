/**
 * @file
 *
 * TODO Add file description
 */

#include <stddef.h> // For NULL
#include <stdlib.h> // For the string parsing functions.
#include <errno.h> // For errno.
#include <stdio.h> // snprintf

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/assert.h>

#include <mouros/mailbox.h> // For the mailboxes.

#include "worker.h" // For the workers.
#include "message_dispatcher.h"
#include "bsp.h" // For bsp_rx_buffer & bsp_tx_buffer.
#include "constants.h"
#include "errors.h"


/**
 * Outgoing error message (error code) queue.
 */
mailbox_t err_msgs;

/**
 * Outgoing normal message queue.
 */
mailbox_t tx_msgs;

struct rx_worker_context {
	char incoming_msg_buf[BSP_MAX_MESSAGE_LENGTH];
	uint32_t pos_in_buf;
	mailbox_t *rx_char_buffer;
	bool msg_is_incoming;
};

static uint8_t rx_worker_stack[TASK_STACK_SIZE];
static worker_t rx_worker;
static struct rx_worker_context rx_context;


static uint8_t tx_worker_stack[TASK_STACK_SIZE];
static worker_t tx_worker;

static void assemble_incoming_message(void *params);
static void check_outgoing_queue(void *params);

static uint8_t calc_checksum(char *buffer, uint32_t len);

static void process_incoming_message(char *message_buf, uint32_t len);

static void process_outgoing_err_message(int32_t err_code,
                                         char *message_buf,
                                         uint32_t len,
                                         mailbox_t *tx_char_buffer);
static void process_outgoing_message(struct message *msg,
                                     char *message_buf,
                                     uint32_t len,
                                     mailbox_t *tx_char_buffer);



static void assemble_incoming_message(void *params)
{
	struct rx_worker_context *context = params;
	uint32_t *pos_in_buf = &(context->pos_in_buf);
	char *incoming_msg_buf = context->incoming_msg_buf;
	mailbox_t *rx_char_buffer = context->rx_char_buffer;
	bool *msg_is_incoming = &(context->msg_is_incoming);

	char ch = '\0';
	bool have_char = false;
	CM_ATOMIC_BLOCK() {
		have_char = os_char_buffer_read_ch(rx_char_buffer, &ch);
	}

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
		// "$<char>*<2byte checksum>\r\n" == 7 bytes
		// We don't save the $ though, so it's 6.
		if (*pos_in_buf >= 6 &&
		    incoming_msg_buf[*pos_in_buf - 2] == '\r' &&
		    incoming_msg_buf[*pos_in_buf - 1] == '\n') {

			incoming_msg_buf[*pos_in_buf - 2] = '\0';
			process_incoming_message(incoming_msg_buf,
						 *pos_in_buf - 1);

			*pos_in_buf = 0;
			*msg_is_incoming = false;
		}

		if (*pos_in_buf >= BSP_MAX_MESSAGE_LENGTH) {
			dispatcher_send_err(INCOMING_MESSAGE_TOO_LONG_ERROR);
			*pos_in_buf = 0;
			*msg_is_incoming = false;
		}
	} else {
		os_task_yield();
	}
}

static void check_outgoing_queue(void *params)
{
	mailbox_t *tx_char_buffer = params;
	char outgoing_msg[BSP_MAX_MESSAGE_LENGTH];

	bool have_msg = false;

	int32_t err_code = NO_ERROR;
	CM_ATOMIC_BLOCK() {
		have_msg = os_mailbox_read(&err_msgs, &err_code);
	}

	if (have_msg) {
		process_outgoing_err_message(err_code,
					     outgoing_msg,
					     BSP_MAX_MESSAGE_LENGTH,
					     tx_char_buffer);

		return;
	}


	struct message *msg = NULL;
	CM_ATOMIC_BLOCK() {
		have_msg = os_mailbox_read(&tx_msgs, &msg);
	}

	if (have_msg) {
		process_outgoing_message(msg,
					 outgoing_msg,
					 BSP_MAX_MESSAGE_LENGTH,
					 tx_char_buffer);
	} else {
		os_task_yield();
	}
}



static uint8_t calc_checksum(char *buffer, uint32_t len)
{
	uint8_t checksum = 0;
	for (uint32_t i = 0; i < len; i++) {
		checksum ^= (uint8_t) buffer[i];
	}

	return checksum;
}

static void process_incoming_message(char *message_buf, uint32_t len)
{
	if (message_buf[len - 4] != '*') {
		dispatcher_send_err(RX_CHECKSUM_ERROR);
		return;
	}

	char *end_ptr = NULL;

	unsigned long msg_csum = strtoul(&message_buf[len - 3], &end_ptr, 16);
	if (end_ptr != &message_buf[len - 1] ||
	    msg_csum != calc_checksum(message_buf, len - 4)) {

		dispatcher_send_err(RX_CHECKSUM_ERROR);
		return;
	}

	message_buf[len - 4] = '\0';
	struct message *msg = msg_parse_message(message_buf);
	if (msg == NULL) {
		dispatcher_send_err(errno);
		return;
	}


	switch (msg->type) {
	case MSG_SET_PLAN:
	case MSG_GET_PLAN:
	case MSG_SET_SPIN_STATE:
	case MSG_GET_SPIN_STATE:
		// TODO send to the spinner subsystem
		break;
	default:
		dispatcher_send_err(MESSAGE_ROUTING_ERROR);
		msg_free_message(msg);
		break;
	}
}


static void process_outgoing_err_message(int32_t err_code,
                                         char *message_buf,
                                         uint32_t len,
                                         mailbox_t *tx_char_buffer)
{
	message_buf[0] = '$';
	uint32_t total_len = 1;

	int payload_len = snprintf(&message_buf[1], len - 1,
	                           "ERROR,%ld", err_code);
	if (payload_len <= 0) {
		return;
	}

	total_len += (uint32_t) payload_len;
	if (total_len >= len) {
		return;
	}

	uint8_t csum = calc_checksum(&message_buf[1], (uint32_t) payload_len);

	int csum_len = snprintf(&message_buf[total_len],
	                        len - total_len,
	                        "*%02X\r\n", csum);

	if (csum_len <= 0) {
		return;
	}

	total_len += (uint32_t) csum_len;
	if (total_len >= len) {
		return;
	}

	os_char_buffer_write_buf_blocking(tx_char_buffer,
	                                  message_buf,
	                                  total_len);
}


static void process_outgoing_message(struct message *msg,
                                     char *message_buf,
                                     uint32_t len,
                                     mailbox_t *tx_char_buffer)
{
	message_buf[0] = '$';
	uint32_t total_len = 1;

	ssize_t payload_len = msg_serialize_message(msg, &message_buf[1],
	                                            (ssize_t) (len - 1));
	if (payload_len <= 0) {
		dispatcher_send_err(errno);
		goto free_msg;
	}


	uint8_t csum = calc_checksum(&message_buf[1], (uint32_t) payload_len);

	total_len += (uint32_t) payload_len;

	int csum_len = snprintf(&message_buf[total_len],
	                        len - total_len,
	                        "*%02X\r\n", csum);

	if (csum_len <= 0) {
		dispatcher_send_err(MESSAGE_FORMATTING_ERROR);
		goto free_msg;
	}

	total_len += (uint32_t) csum_len;
	if (total_len >= len) {
		dispatcher_send_err(MESSAGE_BUF_TOO_SMALL_ERROR);
		goto free_msg;
	}


	if (os_char_buffer_write_buf(tx_char_buffer,
	                             message_buf,
	                             total_len) != total_len) {
		dispatcher_send_err(TX_BUFFER_FULL);
	}


free_msg:
	msg_free_message(msg);
}



void dispatcher_init(void)
{
	// Outgoing error message queue
	static int32_t err_msg_buf[MAX_OUTBOUND_ERROR_MESSAGES];
	os_mailbox_init(&err_msgs,
	                err_msg_buf,
	                MAX_OUTBOUND_ERROR_MESSAGES,
	                sizeof(int32_t),
	                NULL);

	// Outgoing normal message queue
	static struct message *tx_msg_buf[MAX_OUTBOUND_MESSAGES];
	os_mailbox_init(&tx_msgs,
	                tx_msg_buf,
	                MAX_OUTBOUND_MESSAGES,
	                sizeof(struct message *),
	                NULL);

	rx_context.pos_in_buf = 0;
	rx_context.rx_char_buffer = &bsp_rx_buffer;
	rx_context.msg_is_incoming = false;
	worker_task_init(&rx_worker,
	                 "rx_worker",
	                 rx_worker_stack,
	                 TASK_STACK_SIZE,
	                 COMM_TASK_PRIORITY,
	                 assemble_incoming_message,
	                 &rx_context);

	worker_task_init(&tx_worker,
	                 "tx_worker",
	                 tx_worker_stack,
	                 TASK_STACK_SIZE,
	                 COMM_TASK_PRIORITY,
	                 check_outgoing_queue,
	                 &bsp_tx_buffer);

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


bool dispatcher_send_message(struct message *msg)
{
	CM_ATOMIC_CONTEXT();

	return os_mailbox_write(&tx_msgs, &msg);
}


void dispatcher_send_err(int err_code)
{
	while (true) {
		CM_ATOMIC_BLOCK() {
			if (os_mailbox_write(&err_msgs, &err_code)) {
				return;
			}
		}
	}
}

