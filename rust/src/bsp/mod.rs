
#[cfg(feature = "stm32f411discovery")]
mod stm32f411discovery;
#[cfg(feature = "stm32f411discovery")]
pub use self::stm32f411discovery::i2c_periph_addr;

// TODO to be removed once i2c_v2 is implemented
#[cfg(feature = "stm32f411discovery")]
pub mod i2c;
