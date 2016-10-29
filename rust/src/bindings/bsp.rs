#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]


extern crate mouros_rust_bindings;

use self::mouros_rust_bindings::mailbox;


#[derive(Copy, Clone)]
#[repr(u32)]
#[derive(Debug)]
pub enum board_led { LED1 = 0, LED2 = 1, LED3 = 2, LED4 = 3, }


extern "C" {
    pub fn bsp_led_get_state(led: board_led) -> u8;
    pub fn bsp_led_on(led: board_led);
    pub fn bsp_led_off(led: board_led);
    pub fn bsp_led_toggle(led: board_led);
}



extern "C" {
    #![allow(improper_ctypes)]
    pub static mut bsp_rx_buffer: mailbox::Mailbox<u8>;
    pub static mut bsp_tx_buffer: mailbox::Mailbox<u8>;
}
