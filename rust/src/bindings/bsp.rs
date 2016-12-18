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
    #![allow(improper_ctypes)]
    pub static mut bsp_rx_buffer: mailbox::Mailbox<u8>;
    pub static mut bsp_tx_buffer: mailbox::Mailbox<u8>;
}

extern "C" {
    fn bsp_led_get_state(led: i32) -> u8;
    fn bsp_led_on(led: i32);
    fn bsp_led_off(led: i32);
    fn bsp_led_toggle(led: i32);
    fn bsp_stepper_is_moving(stepper_id: u8) -> bool;
    fn bsp_stepper_start(stepper_id: u8);
    fn bsp_stepper_stop(stepper_id: u8);
    fn bsp_stepper_get_pos(stepper_id: u8) -> f32;
    fn bsp_stepper_set_stop_pos(stepper_id: u8, stop_pos_deg: f32);
    fn bsp_stepper_remove_stop_pos(stepper_id: u8);
    fn bsp_stepper_set_rate(stepper_id: u8, rate_pct: f32);
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


#[derive(Clone, Copy)]
pub struct Stepper {
    pub id: u8,
}

impl Stepper {
    pub fn new(id: u8) -> Stepper {
        Stepper { id: id }
    }

    pub fn is_moving(&self) -> bool {
        unsafe { bsp_stepper_is_moving(self.id) }
    }

    pub fn start(&mut self) {
        unsafe { bsp_stepper_start(self.id) }
    }

    pub fn stop(&mut self) {
        unsafe { bsp_stepper_stop(self.id) }
    }

    pub fn get_pos(&self) -> f32 {
        unsafe { bsp_stepper_get_pos(self.id) }
    }

    pub fn set_stop_pos(&mut self, stop_pos_deg: f32) {
        unsafe { bsp_stepper_set_stop_pos(self.id, stop_pos_deg) }
    }

    pub fn remove_stop_pos(&mut self) {
        unsafe { bsp_stepper_remove_stop_pos(self.id) }
    }

    pub fn set_rate(&mut self, rate_pct: f32) {
        unsafe { bsp_stepper_set_rate(self.id, rate_pct) }
    }
}
