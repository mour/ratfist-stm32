
use core::mem;

pub mod i2c_v1;

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

pub fn start_transaction<'a>(device_addr: u8, steps: &mut [Step<'a>]) -> bool {
    if unsafe { it_context.current_transaction.is_some() || it_context.state != PeripheralState::Done(0)} {
        return false;
    }
    
    unsafe {
        it_context.current_transaction = Some(Transaction {
            steps: mem::transmute(steps),
            curr_step: 0,
            device_addr: device_addr
        });
        
        it_context.current_transaction = None;
    }
    
   
    true
}


pub struct InterruptContext<'a, 'b: 'a> {
    current_transaction: Option<Transaction<'a, 'b>>,
    periph_addr: usize,
    state: PeripheralState
}


static mut it_context: InterruptContext = InterruptContext {
    current_transaction: None,
    periph_addr: 0,
    state: PeripheralState::Done(0)
};
