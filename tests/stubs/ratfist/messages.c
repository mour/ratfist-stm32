/**
 * @file
 *
 * This file contains the implementation of the ratfist message factory.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>

#include <ratfist_stubs/messages_stub_helpers.h>

#include "../../../src/messages.h" // Function & struct declarations.


char *ser_output_str = NULL;

void msg_stubs_set_serialization_output(char *output_str)
{
	ser_output_str = output_str;
}


void msg_init(void)
{
}


struct message *msg_parse_message(char *input_buf)
{
	check_expected_ptr(input_buf);

	return mock_ptr_type(struct message *);
}

ssize_t msg_serialize_message(const struct message *msg,
                              char *output_buf,
                              ssize_t output_buf_len)
{
	check_expected_ptr(msg);
	check_expected_ptr(output_buf);
	check_expected(output_buf_len);


	if (ser_output_str != NULL) {
		size_t ser_len = strlen(ser_output_str) + 1;
		if (ser_len > (size_t) output_buf_len) {
			fail_msg("ser_output_str too long to fit in buffer: %d > %d",
					ser_len, output_buf_len);
		}

		strcpy(output_buf, ser_output_str);
	}

	return mock_type(ssize_t);
}

struct message *msg_create_message(enum message_type type)
{
	check_expected(type);

	return mock_type(struct message *);
}

void msg_free_message(struct message *msg)
{
	check_expected_ptr(msg);
}
