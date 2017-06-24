#![allow(dead_code)]

use bsp;

const I2C_CR1_SWRST: u32 = 1 << 15;
const I2C_CR1_ALERT: u32 = 1 << 13;
const I2C_CR1_PEC: u32 = 1 << 12;
const I2C_CR1_POS: u32 = 1 << 11;
const I2C_CR1_ACK: u32 = 1 << 10;
const I2C_CR1_STOP: u32 = 1 << 9;
const I2C_CR1_START: u32 = 1 << 8;
const I2C_CR1_NOSTRETCH: u32 = 1 << 7;
const I2C_CR1_ENGC: u32 = 1 << 6;
const I2C_CR1_ENPEC: u32 = 1 << 5;
const I2C_CR1_ENARP: u32 = 1 << 4;
const I2C_CR1_SMBTYPE: u32 = 1 << 3;
const I2C_CR1_SMBUS: u32 = 1 << 1;
const I2C_CR1_PE: u32 = 1;
const I2C_CR2_LAST: u32 = 1 << 12;
const I2C_CR2_DMAEN: u32 = 1 << 11;
const I2C_CR2_ITBUFEN: u32 = 1 << 10;
const I2C_CR2_ITEVTEN: u32 = 1 << 9;
const I2C_CR2_ITERREN: u32 = 1 << 8;
const I2C_CR2_FREQ_MASK: u32 = 0x3f;
const I2C_OAR1_ADDMODE: u32 = 1 << 15;
const I2C_OAR1_ADD_OFFSET: u32 = 1;
const I2C_OAR1_ALWAYS_ON: u32 = 1 << 14;
const I2C_OAR2_ENDUAL: u32 = 1;
const I2C_OAR2_ADD2_OFFSET: u32 = 1;
const I2C_SR1_SMBALERT: u32 = 1 << 15;
const I2C_SR1_TIMEOUT: u32 = 1 << 14;
const I2C_SR1_PECERR: u32 = 1 << 12;
const I2C_SR1_OVR: u32 = 1 << 11;
const I2C_SR1_AF: u32 = 1 << 10;
const I2C_SR1_ARLO: u32 = 1 << 9;
const I2C_SR1_BERR: u32 = 1 << 8;
const I2C_SR1_TXE: u32 = 1 << 7;
const I2C_SR1_RXNE: u32 = 1 << 6;
const I2C_SR1_STOPF: u32 = 1 << 4;
const I2C_SR1_ADD10: u32 = 1 << 3;
const I2C_SR1_BTF: u32 = 1 << 2;
const I2C_SR1_ADDR: u32 = 1 << 2;
const I2C_SR1_SB: u32 = 1;
const I2C_SR2_DUALF: u32 = 1 << 7;
const I2C_SR2_SMBHOST: u32 = 1 << 6;
const I2C_SR2_SMBDEFAULT: u32 = 1 << 5;
const I2C_SR2_GENCALL: u32 = 1 << 4;
const I2C_SR2_TRA: u32 = 1 << 3;
const I2C_SR2_BUSY: u32 = 1 << 1;
const I2C_SR2_MSL: u32 = 1;
const I2C_CCR_FS: u32 = 1 << 15;
const I2C_CCR_DUTY: u32 = 1 << 14;
const I2C_CCR_CCR_MASK: u32 = 0x0fff;
const I2C_TRISE_TRISE_MASK: u32 = 0x3f;
const I2C_FLTR_ANOFF: u32 = 1 << 4;
const I2C_FLTR_DNF_MASK: u32 = 0x0f;


use super::InterruptContext;
use super::PeripheralState;
use super::Step;

use volatile_register::{RW, RO};

enum CommDir {
    Read,
    Write,
}

#[repr(C)]
pub struct I2C {
    cr1: RW<u32>,
    cr2: RW<u32>,
    oar1: RW<u32>,
    oar2: RW<u32>,
    dr: RW<u32>,
    sr1: RW<u32>,
    sr2: RO<u32>,
    ccr: RW<u32>,
    trise: RW<u32>,
    fltr: RW<u32>,
}

impl I2C {
    pub fn get_periph(periph_addr: usize) -> &'static I2C {
        unsafe { &*(periph_addr as *const I2C) }
    }

    pub fn reset(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_SWRST);
        }
    }

    pub fn enable_periph(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_PE);
        }
    }

    pub fn disable_periph(&self) {
        unsafe {
            self.cr1.modify(|curr| curr & !I2C_CR1_PE);
        }
    }

    pub fn set_100mhz_standard_mode(&self, periph_freq_hz: u32) {
        unsafe {
            let ccr = periph_freq_hz / 2000000;
            let trise = periph_freq_hz / 1000000 + 1;
            let periph_freq_mhz = periph_freq_hz / 1000000;

            self.cr2.modify(|curr| curr | (periph_freq_mhz & I2C_CR2_FREQ_MASK));
            self.ccr.write(ccr & I2C_CCR_CCR_MASK);
            self.trise.write(trise & I2C_TRISE_TRISE_MASK);
        }
    }

    pub fn set_analog_filter_state(&self, new_state: bool) {
        unsafe {
            self.fltr.modify(|curr|
                if new_state {
                    curr & !I2C_FLTR_ANOFF
                } else {
                    curr | I2C_FLTR_ANOFF
                }
            );
        }
    }

    pub fn set_digital_filter_len(&self, new_len: u32) {
        unsafe {
            self.fltr.modify(|curr| curr | (new_len & I2C_FLTR_DNF_MASK));
        }
    }

    fn enable_ack(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_ACK);
        }
    }

    fn disable_ack(&self) {
        unsafe {
            self.cr1.modify(|curr| curr & !I2C_CR1_ACK);
        }
    }

    fn disable_ack_after_next_byte(&self) {
        unsafe {
            self.cr1.modify(|curr| (curr & !I2C_CR1_ACK) | I2C_CR1_POS);
        }
    }

    fn send_start(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_START);
        }
    }

    fn send_stop(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_STOP)
        }
    }

    fn clear_addr_flag(&self) {
        self.sr1.read();
        self.sr2.read();
    }

    fn is_addr_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_ADDR != 0
    }

    fn is_txe_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_TXE != 0
    }

    fn is_rxne_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_RXNE != 0
    }

    fn is_btf_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_BTF != 0
    }

    fn send_addr(&self, addr: u8, dir: CommDir) {
        let dir_bit = match dir {
            CommDir::Read => 1,
            CommDir::Write => 0,
        };

        self.send_data(addr | dir_bit);
    }

    fn read_data(&self) -> u8 {
        self.dr.read() as u8
    }

    fn send_data(&self, data_byte: u8) {
        unsafe {
            self.dr.write(data_byte as u32);
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_i2c_interrupt_handler(ctx: &mut InterruptContext) {

    // Handle interrupts
    // SB - start bit sent - cleared automatically by reading SR1 & writing to DR
    // ADDR - (in master mode) address acked - cleared by reading SR1 & SR2
    // BTF - byte transfer finished - cleared by reading/writing from/to DR
    // RxNE - Rx buffer not empty - cleared by reading from DR
    // TxNE - Tx buffer not empty - cleared by writing to DR
    // Errors:
    // BERR - bus error - misplaced start or stop bit - Cleared by writing 0.
    // ARLO - arbitration lost - cleared by writing 0.
    // AF - acknowledge failure - set when no ACK received. Cleared by writing 0.
    let periph: &I2C = I2C::get_periph(ctx.periph_addr);

    // TODO Abort if an error flag is set

    if let Some(ref mut trans) = ctx.current_transaction {

        match ctx.state {
            PeripheralState::StartBitSet => {
                // Send address
                match trans.steps[trans.curr_step] {
                    Step::Read(_) => periph.send_addr(trans.device_addr, CommDir::Read),
                    Step::Write(_) => periph.send_addr(trans.device_addr, CommDir::Write),
                }
                ctx.state = PeripheralState::AddressSent;
            }

            PeripheralState::AddressSent => {
                if !periph.is_addr_set() {
                    return;
                }

                match trans.steps[trans.curr_step] {
                    Step::Read(_) => {
                        if trans.steps[trans.curr_step].len() == 1 {
                            periph.disable_ack();
                        } else if trans.steps[trans.curr_step].len() == 2 {
                            periph.disable_ack_after_next_byte();
                        } else {
                            periph.enable_ack();
                        }

                        periph.clear_addr_flag();

                        ctx.state = PeripheralState::Receiving(0);
                    }
                    Step::Write(ref data) => {
                        periph.clear_addr_flag();
                        periph.send_data(data[0]);
                        ctx.state = PeripheralState::Transmitting(1);
                    }
                }
            }

            PeripheralState::Transmitting(next_byte_pos) => {
                if (next_byte_pos >= trans.steps[trans.curr_step].len() && !periph.is_btf_set()) ||
                    !periph.is_txe_set() {
                    return;
                }

                if next_byte_pos >= trans.steps[trans.curr_step].len() {
                    if trans.on_last_step() {
                        periph.send_stop();
                        ctx.state = PeripheralState::Done(0);
                    } else {
                        periph.send_start();
                        trans.curr_step += 1;
                        ctx.state = PeripheralState::StartBitSet;
                    }
                } else {
                    periph.send_data(trans.steps[trans.curr_step].as_ref()[next_byte_pos]);
                    ctx.state = PeripheralState::Transmitting(next_byte_pos + 1);
                }
            }

            PeripheralState::Receiving(next_byte_pos) => {
                if next_byte_pos == trans.steps[trans.curr_step].len() - 1 {
                    if !periph.is_btf_set() {
                        return;
                    }

                    periph.send_stop();

                    trans.steps[trans.curr_step].as_mut()[next_byte_pos] = periph.read_data();

                    if trans.on_last_step() {
                        periph.send_stop();
                        ctx.state = PeripheralState::Done(0);
                    } else {
                        periph.send_start();
                        trans.curr_step += 1;
                        ctx.state = PeripheralState::StartBitSet;
                    }

                } else if next_byte_pos == trans.steps[trans.curr_step].len() - 2 {
                    if !periph.is_btf_set() {
                        return;
                    }

                    periph.send_stop();

                    trans.steps[trans.curr_step].as_mut()[next_byte_pos] = periph.read_data();
                    trans.steps[trans.curr_step].as_mut()[next_byte_pos + 1] = periph.read_data();

                    if trans.on_last_step() {
                        periph.send_stop();
                        ctx.state = PeripheralState::Done(0);
                    } else {
                        periph.send_start();
                        trans.curr_step += 1;
                        ctx.state = PeripheralState::StartBitSet;
                    }

                } else {
                    if !periph.is_rxne_set() {
                        return;
                    }

                    if next_byte_pos == trans.steps[trans.curr_step].len() - 3 {
                        periph.disable_ack();
                    }

                    trans.steps[trans.curr_step].as_mut()[next_byte_pos] = periph.read_data();

                    ctx.state = PeripheralState::Receiving(next_byte_pos + 1);
                }
            }

            PeripheralState::Done(_) => {
                // Should not really happen?
            }
        }
    } else {
        // Shouldn't be getting interrupt if no transaction is set. Disable periph.
        periph.reset();
    }
}


pub static mut I2C_PERIPHS: [InterruptContext; 1] = [
    InterruptContext {
        current_transaction: None,
        periph_addr: bsp::i2c_periph_addr::I2C1,
        state: PeripheralState::Done(0),
    },
];



