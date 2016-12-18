#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]

use bindings::messages;

extern "C" {
    fn dispatcher_send_message(msg: *mut messages::message) -> u8;
    fn dispatcher_send_err(err_code: i32);
}

pub fn send_message(msg: messages::MessageWrapper) {
    unsafe {
        dispatcher_send_message(msg.into());
    }
}

pub fn send_error_message(err_code: i32) {
    unsafe { dispatcher_send_err(err_code) }
}
