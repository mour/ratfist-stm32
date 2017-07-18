
#[allow(dead_code)]
pub mod periph_base_addr {
    pub const GPIOA: usize = 0x48000000;
    pub const GPIOB: usize = 0x48000400;
    pub const GPIOC: usize = 0x48000800;
    pub const GPIOD: usize = 0x48000C00;
    pub const GPIOE: usize = 0x48001000;
    pub const GPIOF: usize = 0x48001400;
    pub const RCC: usize = 0x40021000;
    pub const NVIC: usize = 0xE000E100;
}

pub fn board_specific_init() {}
