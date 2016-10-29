#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]

use bindings::messages;

extern "C" {
    pub fn dispatcher_send_message(msg: *mut messages::message) -> u8;
    pub fn dispatcher_send_err(err_code: i32);
}
