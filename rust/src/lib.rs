#![feature(lang_items)]
#![no_std]

#[macro_use]
mod machine;
mod bindings;

pub mod spinner;

#[macro_use]
extern crate mouros_rust_bindings;


#[lang = "panic_fmt"]
#[no_mangle]
pub extern fn rust_begin_panic(_msg: core::fmt::Arguments,
                               _file: &'static str,
                               _line: u32) -> ! {
    loop {}
}