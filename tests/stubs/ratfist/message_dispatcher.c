/**
 * @file
 *
 * This file contains the stub implementation of the message dispatcher.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <ratfist_stubs/message_dispatcher_stub_helpers.h>

#include "../../../src/message_dispatcher.h" // Function & struct declarations.

static struct subsystem_message_conf *last_set_conf = NULL;

void dispatcher_init(void)
{
}

void dispatcher_deinit(void)
{
}

bool dispatcher_register_subsystem(struct subsystem_message_conf *conf)
{
	check_expected_ptr(conf);

	last_set_conf = conf;

	return mock_type(bool);
}

struct subsystem_message_conf *dispatcher_stubs_get_last_conf(void)
{
	return last_set_conf;
}
