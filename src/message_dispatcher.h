/**
 * @file
 *
 * This file contains the declarations for the ratfist message dispatcher.
 */

#ifndef MESSAGE_DISPATCHER_H_
#define MESSAGE_DISPATCHER_H_

#include <stdbool.h> // For bools

#include "messages.h" // For the message factory.

/**
 * Initializes the message dispatcher & RX and TX comm tasks.
 */
void dispatcher_init(void);

/**
 * Sends stop signal to the RX and TX tasks, and blocks until they are stopped.
 */
void dispatcher_deinit(void);

/**
 * Adds the supplied message to the outgoing message queue. Once the message is
 * added, it must not be accessed anymore, as the TX task will deallocate it
 * once it is serialized.
 *
 * @param msg Pointer to the message to be sent.
 *
 * @return True if the message was successfully added to the outgoing message
 *         queue, false otherwise.
 */
bool dispatcher_send_message(struct message *msg);

/**
 * Adds the supplied error code to the outgoing error message queue. This is
 * separate from the normal outgoing message queue, and error messages are sent
 * before any normal messages. The call is blocking, and the addition always
 * succeeds.
 *
 * @param err_code The error code to be used in the outgoing message.
 */
void dispatcher_send_err(int err_code);


#endif /* MESSAGE_DISPATCHER_H_ */


