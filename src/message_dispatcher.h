/**
 * @file
 *
 * This file contains the declarations for the ratfist message dispatcher.
 */

#ifndef MESSAGE_DISPATCHER_H_
#define MESSAGE_DISPATCHER_H_

#include <stdbool.h> // For bools
#include <stdint.h> // For uint32_t, etc.
#include <sys/types.h> // For ssize_t

#include <mouros/mailbox.h> // For mailbox_t

/**
 * Struct representing a message.
 */
struct message {
	/**
	 * The message type ID. This only has to be unique per subsystem.
	 */
	uint32_t type;
	/**
	 * The transaction ID. Gets pre-filled by the message dispatcher for
	 * messages incoming to the MCU. Responses to incoming messages should
	 * have the same transaction ID as the incoming message (must be set in
	 * subsystem).
	 */
	uint32_t transaction_id;
	/**
	 * Pointer to message type specific data.
	 */
	void *data;
};

/**
 * This struct describes a subsystem's messages, their parsing, serialization,
 * allocation, deallocation, and the message queues used for transmitting them
 * to and from the subsystem. It is used to register these functions with
 * the message dispatcher.
 */
struct subsystem_message_conf {
	/**
	 * Pointer to a string with the subsystem name. This must be a unique
	 * identifier, and must be the second token in messages (right after the
	 * transaction ID.
	 */
	char *subsystem_name;

	/**
	 * Pointer to an array of individual message handlers. The position in
	 * this array will be used as the message type for message type structs.
	 */
	struct message_handler {
		/**
		 * Pointer to the message name. This must be the third token in
		 * messages.
		 */
		char *message_name;

		/**
		 * Function used for parsing the payload of the message in
		 * payload_str. May be NULL, in which case the message will not
		 * be parsed, and an error message will be sent back.
		 *
		 * @param msg      Preallocated struct that will be filled with
		 *                 the data from the input string.
		 * @param save_ptr Save pointer for the rest of the message.
		 *                 What is returned by strtok_r after getting
		 *                 the transaction ID, subsystem name, and
		 *                 message name from the raw message string.
		 * @return The parsing outcome. True on success, false on
		 *         failure.
		 */
		bool (*parsing_func)(struct message *msg, char *save_ptr);

		/**
		 * Function used for serializing the payload of the message in
		 * msg. May be NULL, in which case the message will not
		 * be serialized, and an error message will be sent instead.
		 *
		 * @param msg                Pointer to the message struct to be
		 *                           serialized.
		 * @param output_str         Pointer to the string buffer that
		 *                           should hold the serialized payload.
		 * @param output_str_max_len The maximum length of output_str.
		 * @return If successful, returns the number of bytes written.
		 *         Returns -1 if an error occurred during serialization.
		 */
		ssize_t (*serialization_func)(const struct message *msg,
		                              char *output_str,
		                              uint32_t output_str_max_len);
	} *message_handlers;

	/**
	 * The number of handlers in message_handlers[].
	 */
	uint32_t num_message_types;

	/**
	 * Pointer to the dispatcher -> subsystem message struct queue.
	 *
	 * May be NULL.
	 */
	mailbox_t *incoming_msg_queue;

	/**
	 * Pointer to the subsystem -> dispatcher message struct queue.
	 *
	 * May be NULL.
	 */
	mailbox_t *outgoing_msg_queue;

	/**
	 * Priority queue for reporting asynchronous errors. Outgoing messages
	 * are simply int32_t error codes. The dispatcher will serialize them
	 * into message packets.
	 *
	 * May be NULL.
	 */
	mailbox_t *outgoing_err_queue;

	/**
	 * Pointer to the function used for allocating this subsystem's message
	 * structs. This may be NULL if there are no parsing functions, and the
	 * subsystem will only be transmitting messages.
	 *
	 * @param msg_type_id The type of message to allocate. This must
	 *                    correspond to the message position in
	 *                    message_handlers[].
	 * @return Pointer to the newly allocated message struct, or NULL, if
	 *         there was a problem during allocation.
	 */
	struct message *(*alloc_message)(uint32_t msg_type_id);

	/**
	 * Pointer to the function used for freeing this subsystem's message
	 * structs. This may be NULL if there are no serialization functions,
	 * and the subsystem will only be receiving messages.
	 *
	 * @param msg Pointer to the message struct to be deallocated.
	 */
	void (*free_message)(struct message *msg);
};


/**
 * Initializes the message dispatcher & RX and TX comm tasks.
 */
void dispatcher_init(void);

/**
 * Sends stop signal to the RX and TX tasks, and blocks until they are stopped.
 */
void dispatcher_deinit(void);

/**
 * Registers a subsystem's message handlers with the dispatcher.
 *
 * @param conf Pointer to the subsystem message handler configuration struct.
 * @return Returns true if the subsystem was successfully registered, false
 *         otherwise.
 */
bool dispatcher_register_subsystem(struct subsystem_message_conf *conf);


#endif /* MESSAGE_DISPATCHER_H_ */


