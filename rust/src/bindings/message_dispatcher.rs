#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]

use core::mem;
use core::ops::{Deref, DerefMut};
use core::convert::TryFrom;

use mouros_rust_bindings::CVoid;
use mouros_rust_bindings::mailbox::Mailbox;

#[repr(C)]
pub struct message {
    pub msg_type: u32,
    transaction_id: u32,
    pub data: *mut CVoid,
}

#[repr(C)]
pub struct message_handler {
    pub message_name: *const u8,
    pub parsing_func: Option<unsafe extern "C" fn(msg_ptr: *mut message, save_ptr: *mut i8) -> bool>,
    pub serialization_func: Option<unsafe extern "C" fn(msg_ptr: *const message,
                                                        output_str: *mut i8,
                                                        output_str_max_len: u32)
                                                        -> isize>,
}

#[repr(C)]
pub struct subsystem_message_conf {
    pub subsystem_name: *const i8,
    pub message_handlers: *mut message_handler,
    pub num_message_types: u32,
    pub incoming_msg_queue: *mut Mailbox<*mut message>,
    pub outgoing_msg_queue: *mut Mailbox<*mut message>,
    pub outgoing_err_queue: *mut Mailbox<i32>,
    pub alloc_message: Option<unsafe extern "C" fn(msg_type_id: u32) -> *mut message>,
    pub free_message: Option<unsafe extern "C" fn(msg: *mut message)>,
}

extern "C" {
    pub fn dispatcher_register_subsystem(conf: *mut subsystem_message_conf) -> bool;
}





pub struct MessageWrapper<T: Wrappable> {
    raw_msg_ref: *mut message,
    msg: T,
}


pub trait MemManagement {
    fn alloc(msg_type: u32) -> *mut message;
    fn free(msg_ptr: *mut message);
}

pub trait Wrappable: MemManagement + TryFrom<*mut message, Err = ()> {}
impl<T> Wrappable for T where T: MemManagement + TryFrom<*mut message, Err = ()> {}




impl<T> Drop for MessageWrapper<T>
    where T: Wrappable
{
    fn drop(&mut self) {
        T::free(self.raw_msg_ref);
    }
}


impl<T> Deref for MessageWrapper<T>
    where T: Wrappable
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.msg
    }
}

impl<T> DerefMut for MessageWrapper<T>
    where T: Wrappable
{
    fn deref_mut(&mut self) -> &mut T {
        &mut self.msg
    }
}

impl<T> Into<*mut message> for MessageWrapper<T>
    where T: Wrappable
{
    fn into(self) -> *mut message {
        let raw_msg_ptr = self.raw_msg_ref;

        // It's the caller's duty ot deallocate the message now.
        mem::forget(self);

        raw_msg_ptr
    }
}


impl<T> TryFrom<*mut message> for MessageWrapper<T>
    where T: Wrappable
{
    type Err = ();

    fn try_from(raw_msg_ptr: *mut message) -> Result<Self, Self::Err> {
        unsafe {
            if raw_msg_ptr.is_null() || (*raw_msg_ptr).data.is_null() {
                return Err(());
            }
        };

        Ok(MessageWrapper {
               msg: T::try_from(raw_msg_ptr)?,
               raw_msg_ref: raw_msg_ptr,
           })
    }
}


impl<T> MessageWrapper<T>
    where T: Wrappable
{
    pub fn new(trans_id: u32, msg_type: u32) -> Result<MessageWrapper<T>, ()> {

        if let Ok(msg_wrapper) = MessageWrapper::try_from(T::alloc(msg_type)) {
            unsafe {
                (*msg_wrapper.raw_msg_ref).transaction_id = trans_id;
            }

            Ok(msg_wrapper)
        } else {
            Err(())
        }
    }

    pub fn get_transaction_id(&self) -> u32 {
        unsafe { (*self.raw_msg_ref).transaction_id }
    }
}
