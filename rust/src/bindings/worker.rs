
#![allow(dead_code,
         non_camel_case_types,
         non_upper_case_globals,
         non_snake_case)]

pub enum worker {}

extern "C" {
    pub fn worker_start(worker: *mut worker) -> u8;
    pub fn worker_stop(worker: *mut worker);
    pub fn worker_join(worker: *mut worker);
}
