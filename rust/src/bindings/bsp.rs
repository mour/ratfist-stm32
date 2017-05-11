#![allow(dead_code)]

use mouros_rust_bindings::mailbox;

#[allow(non_snake_case)]
pub mod BoardLed {
    pub const LED1: i32 = 0;
    pub const LED2: i32 = 1;
    pub const LED3: i32 = 2;
    pub const LED4: i32 = 3;
}

extern "C" {
    pub static mut bsp_rx_buffer: mailbox::Mailbox<u8>;
    pub static mut bsp_tx_buffer: mailbox::Mailbox<u8>;
}

extern "C" {
    fn bsp_led_get_state(led: i32) -> u8;
    fn bsp_led_on(led: i32);
    fn bsp_led_off(led: i32);
    fn bsp_led_toggle(led: i32);
}


pub fn led_get_state(led: i32) -> bool {
    unsafe { bsp_led_get_state(led) != 0 }
}

pub fn led_on(led: i32) {
    unsafe { bsp_led_on(led) }
}

pub fn led_off(led: i32) {
    unsafe { bsp_led_off(led) }
}

pub fn led_toggle(led: i32) {
    unsafe { bsp_led_toggle(led) }
}
