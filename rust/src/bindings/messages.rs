#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]

use core::mem;
use core::ops::{Deref, DerefMut};

use mouros_rust_bindings::CVoid;
use bindings::constants;

pub mod MsgTypes {
    pub const SET_PLAN: i32 = 0;
    pub const GET_PLAN: i32 = 1;
    pub const SPIN_PLAN_REPLY: i32 = 2;
    pub const SET_SPIN_STATE: i32 = 3;
    pub const GET_SPIN_STATE: i32 = 4;
    pub const SPIN_STATE_REPLY: i32 = 5;
    pub const RET_VAL: i32 = 6;
}


#[repr(C)]
#[derive(Copy)]
pub struct spin_plan_data {
    pub channel_num: u8,
    pub spin_plan_leg_count: u32,
    pub plan_legs: [pwm_leg; constants::MAX_SPIN_PLAN_LEGS as usize],
}
impl ::core::clone::Clone for spin_plan_data {
    fn clone(&self) -> Self {
        *self
    }
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct pwm_leg {
    pub duration_msecs: u32,
    pub target_pct: f32,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct spin_channel {
    pub channel_num: u8,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct ret_val {
    pub ret_val: i32,
}

pub mod SpinStates {
    pub const STOPPED: i32 = 0;
    pub const RUNNING: i32 = 1;
    pub const SPINNING_DOWN: i32 = 2;
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct spin_state_set_data {
    pub channel_num: u8,
    pub state: i32,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct spin_state_data {
    pub channel_num: u8,
    pub state: i32,
    pub plan_time_elapsed_msecs: u32,
    pub output_val_pct: f32,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct message {
    msg_type: i32,
    transaction_id: u32,
    data: *mut CVoid,
}


pub enum Message<'a> {
    SetSpinPlan(&'a mut spin_plan_data),
    GetSpinPlan(&'a mut spin_channel),
    SpinPlanReply(&'a mut spin_plan_data),
    SetSpinState(&'a mut spin_state_set_data),
    GetSpinState(&'a mut spin_channel),
    SpinStateReply(&'a mut spin_state_data),
    ReturnValue(&'a mut ret_val),
}

pub struct MessageWrapper<'a> {
    msg_ptr: *mut message,
    pub payload: Message<'a>,
}

impl<'a> Drop for MessageWrapper<'a> {
    fn drop(&mut self) {
        unsafe { msg_free_message(self.msg_ptr) }
    }
}

impl<'a> Deref for MessageWrapper<'a> {
    type Target = Message<'a>;

    fn deref(&self) -> &Self::Target {
        &self.payload
    }
}

impl<'a> DerefMut for MessageWrapper<'a> {
    fn deref_mut(&mut self) -> &mut Message<'a> {
        &mut self.payload
    }
}

impl<'a> Into<*mut message> for MessageWrapper<'a> {
    fn into(self) -> *mut message {
        let msg_ptr = self.msg_ptr;

        // It's the caller's duty ot deallocate the message now.
        mem::forget(self);

        msg_ptr
    }
}

extern "C" {
    fn msg_create_message(msg_type: i32) -> *mut message;
    fn msg_free_message(msg: *mut message);
}


impl<'a> MessageWrapper<'a> {
    pub fn new(trans_id: u32, msg_type: i32) -> Result<MessageWrapper<'a>, ()> {
        unsafe {
            let mut msg = msg_create_message(msg_type);

            if msg.is_null() {
                return Err(());
            }

            (*msg).transaction_id = trans_id;

            Ok(MessageWrapper::from_raw(msg)?.1)
        }
    }

    pub fn from_raw(msg_ptr: *mut message) -> Result<(u32, MessageWrapper<'a>), ()> {
        unsafe {
            if msg_ptr.is_null() || (*msg_ptr).data.is_null() {
                return Err(());
            }

            let payload = match (*msg_ptr).msg_type {
                MsgTypes::SET_PLAN => {
                    Message::SetSpinPlan(&mut *((*msg_ptr).data as *mut spin_plan_data))
                }
                MsgTypes::GET_PLAN => {
                    Message::GetSpinPlan(&mut *((*msg_ptr).data as *mut spin_channel))
                }
                MsgTypes::SPIN_PLAN_REPLY => {
                    Message::SpinPlanReply(&mut *((*msg_ptr).data as *mut spin_plan_data))
                }
                MsgTypes::SET_SPIN_STATE => {
                    Message::SetSpinState(&mut *((*msg_ptr).data as *mut spin_state_set_data))
                }
                MsgTypes::GET_SPIN_STATE => {
                    Message::GetSpinState(&mut *((*msg_ptr).data as *mut spin_channel))
                }
                MsgTypes::SPIN_STATE_REPLY => {
                    Message::SpinStateReply(&mut *((*msg_ptr).data as *mut spin_state_data))
                }
                MsgTypes::RET_VAL => Message::ReturnValue(&mut *((*msg_ptr).data as *mut ret_val)),
                _ => {
                    msg_free_message(msg_ptr);
                    return Err(());
                }
            };

            Ok(((*msg_ptr).transaction_id,
                MessageWrapper {
                    msg_ptr: msg_ptr,
                    payload: payload,
                }))
        }
    }
}
