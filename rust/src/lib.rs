#![feature(try_from)]
#![no_std]

use core::panic::PanicInfo;

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
extern crate mouros;

extern crate volatile_register;

pub mod bsp;

#[macro_use]
mod bindings;

#[cfg(feature = "spinner")]
pub mod spinner;

#[cfg(feature = "meteo")]
pub mod meteo;


#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
