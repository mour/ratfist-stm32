/**
 * @file
 *
 * This file contains the error codes used in ratfist.
 */

#ifndef ERRORS_H_
#define ERRORS_H_

#define NO_ERROR (0)
#define MEM_ALLOC_ERROR (-1)
#define MESSAGE_PARSING_ERROR (-2)
#define MALFORMED_MESSAGE_ERROR (-3)
#define MESSAGE_BUF_TOO_SMALL_ERROR (-4)
#define UNKNOWN_MESSAGE_TYPE_ERROR (-5)

#define RX_CHECKSUM_ERROR (-6)
#define TX_BUFFER_FULL (-7)
#define RX_BUFFER_FULL (-8)

#define MESSAGE_FORMATTING_ERROR (-9)

#define MESSAGE_ROUTING_ERROR (-10)

#define INCOMING_MESSAGE_TOO_LONG_ERROR (-11)

#endif /* ERRORS_H_ */


