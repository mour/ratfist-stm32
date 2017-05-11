#![feature(try_from)]
#![feature(lang_items)]
#![no_std]

mod bindings;

#[cfg(feature = "spinner")]
pub mod spinner;

#[cfg(feature = "meteo")]
pub mod meteo;

#[macro_use]
extern crate mouros_rust_bindings;


#[lang = "panic_fmt"]
#[no_mangle]
pub extern fn rust_begin_panic(_msg: core::fmt::Arguments,
                               _file: &'static str,
                               _line: u32) -> ! {
    loop {}
}