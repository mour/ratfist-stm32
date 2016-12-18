#![feature(lang_items)]
#![no_std]

#[macro_use]
mod machine;
mod bindings;

pub mod spinner;

#[macro_use]
extern crate mouros_rust_bindings;


#[lang = "panic_fmt"]
extern "C" fn panic_fmt() -> ! {
    loop {
    }
}
