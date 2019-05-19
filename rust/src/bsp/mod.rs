#[cfg(feature = "stm32f411discovery")]
mod stm32f411discovery;
#[cfg(feature = "stm32f411discovery")]
pub use self::stm32f411discovery::periph_base_addr;
#[cfg(feature = "stm32f411discovery")]
use self::stm32f411discovery::board_specific_init;
#[cfg(feature = "stm32f411discovery")]
pub use self::stm32f411discovery::rcc;
#[cfg(feature = "stm32f411discovery")]
pub use self::stm32f411discovery::nvic;

#[cfg(feature = "stm32f072discovery")]
mod stm32f072discovery;
#[cfg(feature = "stm32f072discovery")]
pub use self::stm32f072discovery::periph_base_addr;
#[cfg(feature = "stm32f072discovery")]
use self::stm32f072discovery::board_specific_init;

use super::bindings;
use mouros::diag_led_function;

// TODO to be removed once i2c_v2 is implemented (the switch will be done in the i2c module)
#[cfg(feature = "stm32f411discovery")]
pub mod i2c;

pub mod gpio;
pub mod timer;

fn bsp_diag_led_function(led_state: bool) {
    if led_state {
        bindings::bsp::led_on(bindings::bsp::BoardLed::LED1);
    } else {
        bindings::bsp::led_off(bindings::bsp::BoardLed::LED1);
    }
}

#[no_mangle]
pub extern "C" fn rust_bsp_init() {
    board_specific_init();

    unsafe {
//        diag_led_function = Some(bsp_diag_led_function);
    }

    #[cfg(feature = "stm32f411discovery")]
    i2c::peripheral_init(i2c::Peripheral::I2C3);
}
