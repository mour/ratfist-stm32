
#[cfg(feature = "stm32f411discovery")]
mod stm32f411discovery;
#[cfg(feature = "stm32f411discovery")]
use self::stm32f411discovery::periph_base_addr;
#[cfg(feature = "stm32f411discovery")]
use self::stm32f411discovery::board_specific_init;


// TODO to be removed once i2c_v2 is implemented
#[cfg(feature = "stm32f411discovery")]
pub mod i2c;

pub mod gpio;


#[no_mangle]
pub extern "C" fn rust_bsp_init() {
    board_specific_init();
}