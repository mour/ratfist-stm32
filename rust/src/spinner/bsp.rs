#![allow(dead_code)]

extern "C" {
    fn bsp_stepper_is_moving(stepper_id: u8) -> bool;
    fn bsp_stepper_start(stepper_id: u8);
    fn bsp_stepper_stop(stepper_id: u8);
    fn bsp_stepper_get_pos(stepper_id: u8) -> f32;
    fn bsp_stepper_set_stop_pos(stepper_id: u8, stop_pos_deg: f32);
    fn bsp_stepper_remove_stop_pos(stepper_id: u8);
    fn bsp_stepper_set_rate(stepper_id: u8, rate_pct: f32);
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
