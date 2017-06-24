#![feature(try_from)]
#![feature(lang_items)]
#![no_std]


// TODO Replace with 'matches' once that stops depending on std.
#[macro_export]
macro_rules! matches {
    ($expression:expr, $($pattern:tt)+) => {
        __as_expr! {
            match $expression {
                $($pattern)+ => true,
                _ => false
            }
        }
    }
}

#[doc(hidden)]
#[macro_export]
macro_rules! __as_expr {
    ($value:expr) => ($value)
}


#[macro_use]
extern crate mouros_rust_bindings;

extern crate volatile_register;

pub mod bsp;

mod bindings;

#[cfg(feature = "spinner")]
pub mod spinner;

#[cfg(feature = "meteo")]
pub mod meteo;

#[lang = "panic_fmt"]
#[no_mangle]
pub extern fn rust_begin_panic(_msg: core::fmt::Arguments,
                               _file: &'static str,
                               _line: u32) -> ! {
    loop {}
}
