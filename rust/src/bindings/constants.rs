#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]
pub const BSP_RX_BUFFER_SIZE: u32 = 1000;
pub const BSP_TX_BUFFER_SIZE: u32 = 1000;
pub const BSP_MAX_MESSAGE_LENGTH: u32 = 1000;
pub const MAX_SPIN_PLAN_LEGS: u32 = 100;
pub const MSG_STRUCT_POOL_SIZE: u32 = 35;
pub const SMALL_SIZED_MSG_POOL_SIZE: u32 = 30;
pub const SPIN_PLAN_DATA_MSG_POOL_SIZE: u32 = 5;
pub const MAX_OUTBOUND_ERROR_MESSAGES: u32 = 20;
pub const MAX_OUTBOUND_MESSAGES: u32 = 35;
pub const TASK_STACK_SIZE: u32 = 2000;
pub const COMM_TASK_PRIORITY: u32 = 5;
