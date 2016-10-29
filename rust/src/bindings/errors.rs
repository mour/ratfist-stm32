#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]
pub const NO_ERROR: i32 = 0;
pub const MEM_ALLOC_ERROR: i32 = -1;
pub const MESSAGE_PARSING_ERROR: i32 = -2;
pub const MALFORMED_MESSAGE_ERROR: i32 = -3;
pub const MESSAGE_BUF_TOO_SMALL_ERROR: i32 = -4;
pub const UNKNOWN_MESSAGE_TYPE_ERROR: i32 = -5;
pub const RX_CHECKSUM_ERROR: i32 = -6;
pub const TX_BUFFER_FULL: i32 = -7;
pub const RX_BUFFER_FULL: i32 = -8;
pub const MESSAGE_FORMATTING_ERROR: i32 = -9;
pub const MESSAGE_ROUTING_ERROR: i32 = -10;
pub const INCOMING_MESSAGE_TOO_LONG_ERROR: i32 = -11;
