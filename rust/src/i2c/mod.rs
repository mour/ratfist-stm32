
use core::mem;

pub mod i2c_v1;

use bsp;

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
        self.curr_step >= self.steps.len()
    }
}

pub enum Peripherals {
    I2C1 = 0,
}

pub struct I2C<'a, 'b: 'a> {
    it_context: &'a mut InterruptContext<'a, 'b>,
}

impl<'a, 'b> I2C<'a, 'b> {
    pub fn new(periph: Peripherals) -> I2C<'a, 'b> {
        I2C {
            it_context: unsafe {
                mem::transmute::<&mut InterruptContext, &mut InterruptContext>(
                    &mut I2C_PERIPHS[periph as usize],
                )
            },
        }
    }

    pub fn run_transaction(&mut self, device_addr: u8, steps: &mut [Step]) -> Result<(), ()> {
        unsafe {
            critical!({
                if self.it_context.current_transaction.is_some() ||
                    matches!(self.it_context.state, PeripheralState::Done(_))
                {
                    return Err(());
                }

                let periph = i2c_v1::I2C::get_periph(self.it_context.periph_addr);
                periph.reset();

                self.it_context.current_transaction = Some(Transaction {
                    steps: mem::transmute::<&mut [Step], &mut [Step]>(steps),
                    curr_step: 0,
                    device_addr: device_addr,
                });
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
                        },
                        _ => {}
                    }
                });
            }
        }
    }
}


pub struct InterruptContext<'a, 'b: 'a> {
    current_transaction: Option<Transaction<'a, 'b>>,
    periph_addr: usize,
    state: PeripheralState,
}

static mut I2C_PERIPHS: [InterruptContext; 1] = [
    InterruptContext {
        current_transaction: None,
        periph_addr: bsp::i2c::I2C1,
        state: PeripheralState::Done(0),
    },
];
