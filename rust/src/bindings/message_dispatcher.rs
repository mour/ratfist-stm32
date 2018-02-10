#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]

use core::mem;
use core::ops::{Deref, DerefMut};
pub use core::convert::TryFrom;

use core::fmt;
use core::ptr;

use core::slice;
use core::str;

use mouros::CVoid;
use mouros::mailbox::MailboxRaw;

#[repr(C)]
pub struct message {
    pub msg_type: u32,
    transaction_id: u32,
    pub data: *mut CVoid,
}


#[repr(C)]
pub struct message_handler {
    pub message_name: *const u8,
    pub parsing_func:
        Option<unsafe extern "C" fn(msg_ptr: *mut message, save_ptr: *mut u8) -> bool>,
    pub serialization_func: Option<
        unsafe extern "C" fn(msg_ptr: *const message,
                             output_str: *mut u8,
                             output_str_max_len: u32)
                             -> isize,
    >,
}

#[repr(C)]
pub struct subsystem_message_conf {
    pub subsystem_name: *const u8,
    pub message_handlers: *const message_handler,
    pub num_message_types: u32,
    pub incoming_msg_queue: *mut MailboxRaw,
    pub outgoing_msg_queue: *mut MailboxRaw,
    pub outgoing_err_queue: *mut MailboxRaw,
    pub alloc_message: Option<unsafe extern "C" fn(msg_type_id: u32) -> *mut message>,
    pub free_message: Option<unsafe extern "C" fn(msg: *mut message)>,
}

extern "C" {
    pub fn dispatcher_register_subsystem(conf: *mut subsystem_message_conf) -> bool;
}





pub struct MessageWrapper<T: Wrappable> {
    raw_msg_ptr: *mut message,
    msg: T,
}


pub trait MemManagement {
    fn alloc(msg_type: u32) -> *mut message;
    fn free(msg_ptr: *mut message);
}

pub trait Wrappable: MemManagement + TryFrom<*mut message, Error = ()> {}
impl<T> Wrappable for T
where
    T: MemManagement + TryFrom<*mut message, Error = ()>,
{
}




impl<T> Drop for MessageWrapper<T>
where
    T: Wrappable,
{
    fn drop(&mut self) {
        T::free(self.raw_msg_ptr);
    }
}


impl<T> Deref for MessageWrapper<T>
where
    T: Wrappable,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.msg
    }
}

impl<T> DerefMut for MessageWrapper<T>
where
    T: Wrappable,
{
    fn deref_mut(&mut self) -> &mut T {
        &mut self.msg
    }
}

impl<T> Into<*mut message> for MessageWrapper<T>
where
    T: Wrappable,
{
    fn into(self) -> *mut message {
        let raw_msg_ptr = self.raw_msg_ptr;

        // It's the caller's duty ot deallocate the message now.
        mem::forget(self);

        raw_msg_ptr
    }
}


impl<T> TryFrom<*mut message> for MessageWrapper<T>
where
    T: Wrappable,
{
    type Error = ();

    fn try_from(raw_msg_ptr: *mut message) -> Result<Self, Self::Error> {
        unsafe {
            if raw_msg_ptr.is_null() || (*raw_msg_ptr).data.is_null() {
                return Err(());
            }
        };

        Ok(MessageWrapper {
            msg: T::try_from(raw_msg_ptr)?,
            raw_msg_ptr: raw_msg_ptr,
        })
    }
}


impl<T> MessageWrapper<T>
where
    T: Wrappable,
{
    pub fn new(trans_id: u32, msg_type: u32) -> Result<MessageWrapper<T>, ()> {

        if let Ok(msg_wrapper) = MessageWrapper::try_from(T::alloc(msg_type)) {
            unsafe {
                (*msg_wrapper.raw_msg_ptr).transaction_id = trans_id;
            }

            Ok(msg_wrapper)
        } else {
            Err(())
        }
    }

    pub fn get_transaction_id(&self) -> u32 {
        unsafe { (*self.raw_msg_ptr).transaction_id }
    }
}



#[macro_export]
macro_rules! derive_message_wrapper {
    ($type_name:tt<$($lifetime:tt),+>, $msg_id:ident => ( $($parsing_expr:tt)+ ), $alloc_func:expr, $free_func:expr) => (
        derive_message_wrapper!(
            $type_name<$($lifetime),+>,
            [$msg_id => ( $($parsing_expr)+ )],
            $alloc_func, $free_func);
    );

    ($type_name:tt<$($lifetime:tt),+>, [$( $msg_id:ident => ( $($parsing_expr:tt)+ ) ),+], $alloc_func:expr, $free_func:expr) => (
        impl<$($lifetime),+> $crate::bindings::message_dispatcher::TryFrom<*mut $crate::bindings::message_dispatcher::message> for $type_name<$($lifetime),+> {
            derive_message_wrapper!(@try_from_impl
                $type_name<$($lifetime),+>,
                [$( $msg_id => ( $($parsing_expr)+ ) ),+]);
        }

        impl<$($lifetime),+> $crate::bindings::message_dispatcher::MemManagement for $type_name<$($lifetime),+> {
            derive_message_wrapper!(@mem_mgnt_impl $alloc_func, $free_func);
        }
    );


    ($type_name:ty, $msg_id:ident => ( $($parsing_expr:tt)+ ), $alloc_func:expr, $free_func:expr) => (
        derive_message_wrapper!(
            $type_name,
            [$msg_id => ( $($parsing_expr)+ )], $alloc_func, $free_func);
    );

    ($type_name:ty, [$( $msg_id:ident => ( $($parsing_expr:tt)+ ) ),+], $alloc_func:expr, $free_func:expr) => (
        impl $crate::bindings::message_dispatcher::TryFrom<*mut $crate::bindings::message_dispatcher::message> for $type_name {
            derive_message_wrapper!(@try_from_impl
                $type_name,
                [$($msg_id => ( $($parsing_expr)+ ) ),+]);
        }

        impl $crate::bindings::message_dispatcher::MemManagement for $type_name {
            derive_message_wrapper!(@mem_mgnt_impl $alloc_func, $free_func);
        }
    );



    (@try_from_impl $type_name:ty, [$($msg_id:ident => ( $($parsing_expr:tt)+ ) ),+]) => (
        type Error = ();

        fn try_from(raw_ptr: *mut $crate::bindings::message_dispatcher::message) -> Result<$type_name, Self::Error> {
            unsafe {
                if raw_ptr.is_null() {
                    return Err(());
                }

                #[allow(unused_variables)]
                let data_ptr = (*raw_ptr).data;

                match (*raw_ptr).msg_type {
                    $($msg_id => {derive_message_wrapper!(@match_pattern data_ptr, $($parsing_expr)+ )},)+
                    _ => Err(())
                }
            }
        }
    );

    (@match_pattern $_payload_ptr:expr, $type_head:tt$(::$type_tail:tt)* ) => {
        Ok($type_head$(::$type_tail)*)
    };


    (@match_pattern $payload_ptr:expr, $($parsing_expr:tt)+ ) => {
        {
            if $payload_ptr.is_null() {
                return Err(());
            }

            derive_message_wrapper!(@as_expr $($parsing_expr)+)($payload_ptr)
        }
    };

    (@mem_mgnt_impl $alloc_func:expr, $free_func:expr) => {
        fn alloc(msg_type: u32) -> *mut $crate::bindings::message_dispatcher::message {
            #[allow(unused_unsafe)]
            unsafe { $alloc_func(msg_type) }
        }

        fn free(msg_ptr: *mut $crate::bindings::message_dispatcher::message) {
            #[allow(unused_unsafe)]
            unsafe { $free_func(msg_ptr); }
        }
    };

    (@as_expr $e:expr) => {$e};
}




pub struct SyncMemory<T>(T);
unsafe impl<T> Sync for SyncMemory<T> {}

impl<T> SyncMemory<T> {
    pub fn new(item: T) -> SyncMemory<T> {
        SyncMemory(item)
    }
}

impl<T> AsRef<T> for SyncMemory<T> {
    fn as_ref(&self) -> &T {
        &self.0
    }
}

impl<T> AsMut<T> for SyncMemory<T> {
    fn as_mut(&mut self) -> &mut T {
        &mut self.0
    }
}

#[macro_export]
macro_rules! unwrap_or_ret_false {
    ($wrapped:expr) => {
        if let Ok(unwrapped) = $wrapped {
            unwrapped
        } else {
            return false;
        }
    }
}

pub struct CStrWriter<'buf> {
    buf: &'buf mut [u8],
    pos: usize,
}

impl<'buf> CStrWriter<'buf> {
    pub fn new(buf: &mut [u8]) -> CStrWriter {
        buf[0] = '\0' as u8;

        CStrWriter { buf: buf, pos: 0 }
    }

    pub fn len(&self) -> usize {
        self.pos
    }
}

impl<'buf> fmt::Write for CStrWriter<'buf> {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let s_len = s.as_bytes().len();

        if (s_len + 1) > (self.buf.len() - self.pos) {
            return Err(fmt::Error);
        }

        unsafe {
            ptr::copy_nonoverlapping(
                s.as_ptr(),
                self.buf.as_mut_ptr().offset(self.pos as isize),
                s_len,
            );
        }

        self.pos += s_len;
        self.buf[self.pos] = '\0' as u8;

        Ok(())
    }
}

pub unsafe fn cstr_ptr_to_str_slice<'a>(cstr_ptr: *mut u8) -> Result<&'a mut str, str::Utf8Error> {
    let mut len = 0;
    while *cstr_ptr.offset(len) != b'\0' {
        len += 1;
    }

    str::from_utf8_mut(slice::from_raw_parts_mut(cstr_ptr, len as usize))
}
