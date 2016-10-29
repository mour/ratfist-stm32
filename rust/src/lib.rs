#![feature(lang_items)]
#![no_std]

mod bindings;

#[macro_use]
mod machine;

pub mod spinner;
pub use spinner::*;

#[lang = "panic_fmt"]
extern fn panic_fmt() -> ! {
    loop {}
}
