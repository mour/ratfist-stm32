/**
 * @file
 *
 * TODO Add file description.
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#ifndef BSP_RX_BUFFER_SIZE
/** The size in bytes of the UART RX character buffer. See bsp_rx_buffer. */
#define BSP_RX_BUFFER_SIZE 1000
#endif

#ifndef BSP_TX_BUFFER_SIZE
/** The size in bytes of the UART TX character buffer. See bsp_tx_buffer. */
#define BSP_TX_BUFFER_SIZE 1000
#endif



#ifndef MAX_SPIN_PLAN_LEGS
/** The maximum number of legs any one spin plan may contain. */
#define MAX_SPIN_PLAN_LEGS 100
#endif



#ifndef EVENT_STRUCT_POOL_SIZE
/**
 * The number of event structs that may be allocated (i.e. waiting to be
 * processed) at any given time.
 */
#define EVENT_STRUCT_POOL_SIZE 35
#endif

#ifndef SMALL_SIZED_EVENT_POOL_SIZE
/**
 * The number of event data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of events:
 *  - GET_PLAN
 *  - SET_SPIN_STATE
 *  - GET_SPIN_STATE
 *  - SPIN_STATE_REPLY
 */
#define SMALL_SIZED_EVENT_POOL_SIZE 30
#endif

#ifndef SPIN_PLAN_DATA_EVENT_POOL_SIZE
/**
 * The number of event data structs that may be allocated (i.e. waiting to be
 * processed) at any given time, for the following types of events:
 *  - SET_PLAN
 *  - SPIN_PLAN_REPLY
 */
#define SPIN_PLAN_DATA_EVENT_POOL_SIZE 5
#endif


#endif /* CONSTANTS_H_ */


