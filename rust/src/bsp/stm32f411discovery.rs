
#[allow(dead_code)]
pub mod periph_base_addr {
    pub const I2C1: usize = 1073763328;
    pub const I2C2: usize = 1073764352;
    pub const I2C3: usize = 1073765376;
    pub const GPIOA: usize = 0x40020000;
    pub const GPIOB: usize = 0x40020400;
    pub const GPIOC: usize = 0x40020800;
    pub const GPIOD: usize = 0x40020C00;
    pub const GPIOE: usize = 0x40021000;
    pub const GPIOH: usize = 0x40021C00;
}

pub fn board_specific_init() {
}