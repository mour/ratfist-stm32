
use core::mem;

pub mod i2c_v1;

use bsp;

#[derive(PartialEq)]
enum PeripheralState {
    StartBitSet,
    AddressSent,
    Transmitting(usize),
    Receiving(usize),
    Done(i32)
}


pub enum Step<'a> {
    Read(&'a mut [u8]),
    Write(&'a mut [u8]),
}

impl<'a> AsMut<[u8]> for Step<'a> {
    fn as_mut(&mut self) -> &mut [u8] {
        match *self {
            Step::Read(ref mut data) | Step::Write(ref mut data) => data,
        }
    }
}

impl<'a> AsRef<[u8]> for Step<'a> {
    fn as_ref(&self) -> &[u8] {
        match *self {
            Step::Read(ref data) | Step::Write(ref data) => data,
        }
    }
}

impl<'a> Step<'a> {
    fn len(&self) -> usize {
        match *self {
            Step::Read(ref data) => data.len(),
            Step::Write(ref data) => data.len()
        }
    }
}


struct Transaction<'a, 'b: 'a> {
    steps: &'a mut [Step<'b>],
    curr_step: usize,
    device_addr: u8
}

impl<'a, 'b> Transaction<'a, 'b> {
    fn on_last_step(&self) -> bool {
        self.curr_step >= self.steps.len()
    }
}

pub enum Peripheral {
    I2C1
}

impl Into<usize> for Peripheral {
    fn into(self) -> usize {
        match self {
            Peripheral::I2C1 => bsp::i2c::I2C1
        }
    }
}

pub fn run_transaction<'a>(device_addr: u8, steps: &mut [Step<'a>]) -> bool {
    unsafe {
        critical!({
            if IT_CONTEXT.current_transaction.is_some() || matches!(IT_CONTEXT.state, PeripheralState::Done(_)) {
                return false;
            }

            let periph = i2c_v1::I2C::get_periph(IT_CONTEXT.periph_addr);
            periph.reset();

            IT_CONTEXT.current_transaction = Some(Transaction {
                steps: mem::transmute::<&mut [Step], &mut [Step]>(steps),
                curr_step: 0,
                device_addr: device_addr
            });
        });

        loop {
            critical!({
                match IT_CONTEXT.state {
                    PeripheralState::Done(ret_val) => {
                        IT_CONTEXT.current_transaction = None;
                        return ret_val == 0;
                    },
                    _ => {}
                }
            });
        }
    }
}


pub struct InterruptContext<'a, 'b: 'a> {
    current_transaction: Option<Transaction<'a, 'b>>,
    periph_addr: usize,
    state: PeripheralState
}


static mut IT_CONTEXT: InterruptContext = InterruptContext {
    current_transaction: None,
    periph_addr: bsp::i2c::I2C1,
    state: PeripheralState::Done(0)
};
