
#[cfg(feature = "stm32f411discovery")]
mod i2c_v1;
#[cfg(feature = "stm32f411discovery")]
use self::i2c_v1 as i2c_impl;

pub use self::i2c_impl::{rust_i2c_interrupt_handler, rust_i2c_interrupt_error_handler};
pub use self::i2c_impl::peripheral_init;

use core::mem;

#[derive(PartialEq)]
enum PeripheralState {
    StartBitSet,
    AddressSent,
    Transmitting(usize),
    Receiving(usize),
    Done(i32),
}


pub enum Step<'a> {
    Read(&'a mut [u8]),
    Write(&'a mut [u8]),
}

impl<'a> AsMut<[u8]> for Step<'a> {
    fn as_mut(&mut self) -> &mut [u8] {
        match *self {
            Step::Read(ref mut data) |
            Step::Write(ref mut data) => data,
        }
    }
}

impl<'a> AsRef<[u8]> for Step<'a> {
    fn as_ref(&self) -> &[u8] {
        match *self {
            Step::Read(ref data) |
            Step::Write(ref data) => data,
        }
    }
}

impl<'a> Step<'a> {
    fn len(&self) -> usize {
        match *self {
            Step::Read(ref data) => data.len(),
            Step::Write(ref data) => data.len(),
        }
    }
}


struct Transaction<'a, 'b: 'a> {
    steps: &'a mut [Step<'b>],
    curr_step: usize,
    device_addr: u8,
}

impl<'a, 'b> Transaction<'a, 'b> {
    fn on_last_step(&self) -> bool {
        self.curr_step + 1 >= self.steps.len()
    }
}

#[derive(Clone, Copy)]
#[repr(u32)]
pub enum Peripheral {
    I2C1 = 0,
    I2C2 = 1,
    I2C3 = 2,
}

impl<'a, 'b> Peripheral {
    fn get_ctx(&self) -> &'a mut InterruptContext<'a, 'b> {
        unsafe {
            mem::transmute::<&mut InterruptContext, &mut InterruptContext>(
                &mut i2c_impl::I2C_PERIPHS[*self as usize],
            )
        }
    }

    pub fn run_transaction(&self, device_addr: u8, steps: &mut [Step]) -> Result<(), ()> {
        unsafe {
            let ctx = self.get_ctx();

            critical!({
                if ctx.current_transaction.is_some() {
                    return Err(());
                }

                if !matches!(ctx.state, PeripheralState::Done(_)) {
                    return Err(());
                }

                let periph = i2c_impl::I2C::get_periph(ctx.periph_id);
                periph.reset();

                ctx.current_transaction = Some(Transaction {
                    steps: mem::transmute::<&mut [Step], &mut [Step]>(steps),
                    curr_step: 0,
                    device_addr: device_addr,
                });

                ctx.state = PeripheralState::StartBitSet;
                periph.send_start();
            });

            loop {
                critical!({
                    match ctx.state {
                        PeripheralState::Done(ret_val) => {
                            ctx.current_transaction = None;
                            if ret_val == 0 {
                                return Ok(());
                            } else {
                                i2c_impl::deblock_bus(ctx.periph_id);
                                return Err(());
                            }
                        }
                        _ => {}
                    }
                });
            }
        }
    }
}

pub struct InterruptContext<'a, 'b: 'a> {
    current_transaction: Option<Transaction<'a, 'b>>,
    periph_id: Peripheral,
    state: PeripheralState,
}
