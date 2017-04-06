/**
 * @file
 *
 * This file contains the error codes used in ratfist.
 */

#ifndef ERRORS_H_
#define ERRORS_H_

#define NO_ERROR (0)

#define RX_CHECKSUM_ERROR (-1)
#define MESSAGE_PARSING_ERROR (-2)
#define MESSAGE_FORMATTING_ERROR (-3)

#define MEM_ALLOC_ERROR (-4)
#define MESSAGE_ROUTING_ERROR (-5)

#define UNKNOWN_SUBSYSTEM_ERROR (-6)
#define UNKNOWN_MESSAGE_TYPE_ERROR (-7)
#define MISSING_MESSAGE_HANDLER_ERROR (-8)

#define TX_BUFFER_FULL (-9)
#define RX_BUFFER_FULL (-10)

#define MESSAGE_TOO_LONG_ERROR (-11)

#endif /* ERRORS_H_ */


