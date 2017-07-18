

#[cfg(feature = "stm32f411discovery")]
mod i2c_v1;
#[cfg(feature = "stm32f411discovery")]
use self::i2c_v1 as i2c_impl;

pub use self::i2c_impl::rust_i2c_interrupt_handler;
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

pub struct I2C<'a, 'b: 'a> {
    it_context: &'a mut InterruptContext<'a, 'b>,
}

impl<'a, 'b> I2C<'a, 'b> {
    pub fn new(periph: Peripheral) -> I2C<'a, 'b> {
        I2C {
            it_context: unsafe {
                mem::transmute::<&mut InterruptContext, &mut InterruptContext>(
                    &mut i2c_impl::I2C_PERIPHS[periph as usize],
                )
            },
        }
    }

    pub fn run_transaction(&mut self, device_addr: u8, steps: &mut [Step]) -> Result<(), ()> {
        unsafe {
            critical!({
                if self.it_context.current_transaction.is_some() {
                    return Err(());
                }

                if !matches!(self.it_context.state, PeripheralState::Done(_)) {
                    return Err(());
                }

                let periph = i2c_impl::I2C::get_periph(self.it_context.periph_id);
                periph.reset();

                self.it_context.current_transaction = Some(Transaction {
                    steps: mem::transmute::<&mut [Step], &mut [Step]>(steps),
                    curr_step: 0,
                    device_addr: device_addr,
                });

                self.it_context.state = PeripheralState::StartBitSet;
                periph.send_start();
            });

            loop {
                critical!({
                    match self.it_context.state {
                        PeripheralState::Done(ret_val) => {
                            self.it_context.current_transaction = None;
                            if ret_val == 0 {
                                return Ok(());
                            } else {
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
