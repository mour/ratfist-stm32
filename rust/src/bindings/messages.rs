#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]


extern crate mouros_rust_bindings;

use self::mouros_rust_bindings::CVoid;
use super::constants;

#[derive(Copy, Clone)]
#[repr(u32)]
#[derive(Debug)]
pub enum message_type {
    MSG_SET_PLAN = 0,
    MSG_GET_PLAN = 1,
    MSG_SPIN_PLAN_REPLY = 2,
    MSG_SET_SPIN_STATE = 3,
    MSG_GET_SPIN_STATE = 4,
    MSG_SPIN_STATE_REPLY = 5,
    MSG_NUM_MESSAGE_TYPES = 6,
}

#[repr(C)]
#[derive(Copy)]
pub struct spin_plan_data {
    pub channel_num: u8,
    pub spin_plan_leg_count: u32,
    pub plan_legs: [pwm_leg; constants::MAX_SPIN_PLAN_LEGS as usize],
}
impl ::core::clone::Clone for spin_plan_data {
    fn clone(&self) -> Self { *self }
}
impl ::core::default::Default for spin_plan_data {
    fn default() -> Self { unsafe { ::core::mem::zeroed() } }
}

#[repr(C)]
#[derive(Copy, Clone)]
#[derive(Debug)]
pub struct pwm_leg {
    pub duration_msecs: u32,
    pub target_pct: f32,
}
impl ::core::default::Default for pwm_leg {
    fn default() -> Self { unsafe { ::core::mem::zeroed() } }
}

#[repr(C)]
#[derive(Copy, Clone)]
#[derive(Debug)]
pub struct spin_plan_channel {
    pub channel_num: u8,
}
impl ::core::default::Default for spin_plan_channel {
    fn default() -> Self { unsafe { ::core::mem::zeroed() } }
}

#[derive(Copy, Clone)]
#[repr(u32)]
#[derive(Debug)]
pub enum spin_state {
    SPIN_STOPPED = 0,
    SPIN_RUNNING = 1,
    SPIN_SPINNING_DOWN = 2,
    SPIN_NUM_SPIN_STATES = 3,
}

#[repr(C)]
#[derive(Copy, Clone)]
#[derive(Debug)]
pub struct spin_state_set_data {
    pub channel_num: u8,
    pub state: spin_state,
}
impl ::core::default::Default for spin_state_set_data {
    fn default() -> Self { unsafe { ::core::mem::zeroed() } }
}

#[repr(C)]
#[derive(Copy, Clone)]
#[derive(Debug)]
pub struct spin_state_data {
    pub channel_num: u8,
    pub state: spin_state,
    pub plan_time_elapsed_msecs: u64,
    pub output_val_pct: f32,
    _bindgen_padding_0_: [u8; 4usize],
}
impl ::core::default::Default for spin_state_data {
    fn default() -> Self { unsafe { ::core::mem::zeroed() } }
}

#[repr(C)]
#[derive(Copy, Clone)]
#[derive(Debug)]
pub struct message {
    pub type_: message_type,
    pub data: *mut CVoid,
}
impl ::core::default::Default for message {
    fn default() -> Self { unsafe { ::core::mem::zeroed() } }
}


extern "C" {
    pub fn msg_create_message(type_: message_type) -> *mut message;
    pub fn msg_free_message(msg: *mut message);
}
