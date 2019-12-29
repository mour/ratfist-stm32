#![allow(dead_code)]

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
const I2C_SR1_ADDR: u32 = 1 << 1;
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
use super::Peripheral;

use crate::bsp;
use crate::bsp::gpio;
use crate::bsp::rcc;
use crate::bsp::nvic;

use mouros::tasks;

use volatile_register::{RO, RW};

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

pub fn peripheral_init(periph: Peripheral) {
    let i2c = I2C::get_periph(periph);

    match periph {
        Peripheral::I2C1 => {
            rcc::enable_periph(rcc::Peripheral::GpioB);

            let sda = gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin9);
            let scl = gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin6);

            sda.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
            sda.set_mode(gpio::Mode::AlternateFunction(
                gpio::AltFuncNum::Af4,
                gpio::OutputType::OpenDrain,
                gpio::OutputSpeed::High,
            ));

            scl.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
            scl.set_mode(gpio::Mode::AlternateFunction(
                gpio::AltFuncNum::Af4,
                gpio::OutputType::OpenDrain,
                gpio::OutputSpeed::High,
            ));

            rcc::enable_periph(rcc::Peripheral::I2C1);
            nvic::enable_interrupt(nvic::Interrupt::I2C1Ev);
            nvic::set_priority(nvic::Interrupt::I2C1Ev, 1);
            nvic::set_subpriority(nvic::Interrupt::I2C1Ev, 1);

            nvic::enable_interrupt(nvic::Interrupt::I2C1Er);
            nvic::set_priority(nvic::Interrupt::I2C1Er, 1);
            nvic::set_subpriority(nvic::Interrupt::I2C1Er, 0);
        }
        Peripheral::I2C2 => {
            rcc::enable_periph(rcc::Peripheral::GpioB);

            let sda = gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin3);
            let scl = gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin10);

            sda.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
            sda.set_mode(gpio::Mode::AlternateFunction(
                gpio::AltFuncNum::Af4,
                gpio::OutputType::OpenDrain,
                gpio::OutputSpeed::High,
            ));

            scl.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
            scl.set_mode(gpio::Mode::AlternateFunction(
                gpio::AltFuncNum::Af4,
                gpio::OutputType::OpenDrain,
                gpio::OutputSpeed::High,
            ));

            rcc::enable_periph(rcc::Peripheral::I2C2);
            nvic::enable_interrupt(nvic::Interrupt::I2C2Ev);
            nvic::set_priority(nvic::Interrupt::I2C2Ev, 1);
            nvic::set_subpriority(nvic::Interrupt::I2C2Ev, 1);

            nvic::enable_interrupt(nvic::Interrupt::I2C2Er);
            nvic::set_priority(nvic::Interrupt::I2C2Er, 1);
            nvic::set_subpriority(nvic::Interrupt::I2C2Er, 0);
        }
        Peripheral::I2C3 => {
            rcc::enable_periph(rcc::Peripheral::GpioA);
            rcc::enable_periph(rcc::Peripheral::GpioC);

            let sda = gpio::Pin::new(gpio::Gpio::GpioC, gpio::PinNum::Pin9);
            let scl = gpio::Pin::new(gpio::Gpio::GpioA, gpio::PinNum::Pin8);

            sda.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
            sda.set_mode(gpio::Mode::AlternateFunction(
                gpio::AltFuncNum::Af4,
                gpio::OutputType::OpenDrain,
                gpio::OutputSpeed::High,
            ));

            scl.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
            scl.set_mode(gpio::Mode::AlternateFunction(
                gpio::AltFuncNum::Af4,
                gpio::OutputType::OpenDrain,
                gpio::OutputSpeed::High,
            ));

            rcc::enable_periph(rcc::Peripheral::I2C3);
            nvic::enable_interrupt(nvic::Interrupt::I2C3Ev);
            nvic::set_priority(nvic::Interrupt::I2C3Ev, 1);
            nvic::set_subpriority(nvic::Interrupt::I2C3Ev, 1);

            nvic::enable_interrupt(nvic::Interrupt::I2C3Er);
            nvic::set_priority(nvic::Interrupt::I2C3Er, 1);
            nvic::set_subpriority(nvic::Interrupt::I2C3Er, 0);
        }
    }

    i2c.reset();

    i2c.set_100mhz_standard_mode(48000000);
    i2c.enable_event_interrupts();
    i2c.enable_error_interrupts();
    i2c.set_analog_filter_state(true);
    i2c.set_digital_filter_len(0x0f);

    i2c.enable_periph();
}

pub fn deblock_bus(periph: Peripheral) {
    let i2c = I2C::get_periph(periph);
    i2c.reset();

    let (sda, scl) = match periph {
        Peripheral::I2C1 => (
            gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin9),
            gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin6),
        ),
        Peripheral::I2C2 => (
            gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin3),
            gpio::Pin::new(gpio::Gpio::GpioB, gpio::PinNum::Pin10),
        ),
        Peripheral::I2C3 => (
            gpio::Pin::new(gpio::Gpio::GpioC, gpio::PinNum::Pin9),
            gpio::Pin::new(gpio::Gpio::GpioA, gpio::PinNum::Pin8),
        ),
    };

    sda.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
    sda.set_mode(gpio::Mode::Output(
        gpio::OutputType::OpenDrain,
        gpio::OutputSpeed::High,
    ));

    scl.set_pullup_pulldown_mode(gpio::PullUpDown::PullUp);
    scl.set_mode(gpio::Mode::Output(
        gpio::OutputType::OpenDrain,
        gpio::OutputSpeed::High,
    ));

    // Generate nine clock pulses while SDA is high.
    // The pulses should be at 100kHz -> 5us high, 5us low. The wait_us function seems to have about a 3us overhead.
    sda.set();
    for _ in 0..9 {
        scl.set();
        tasks::wait_us(2);
        scl.reset();
        tasks::wait_us(2);
    }
    scl.set();

    peripheral_init(periph);
}

impl I2C {
    pub fn get_periph(periph: Peripheral) -> &'static I2C {
        let periph_addr = match periph {
            Peripheral::I2C1 => bsp::periph_base_addr::I2C1,
            Peripheral::I2C2 => bsp::periph_base_addr::I2C2,
            Peripheral::I2C3 => bsp::periph_base_addr::I2C3,
        };
        unsafe { &*(periph_addr as *const I2C) }
    }

    pub fn reset(&self) {
        unsafe {
            self.clear_transaction_ctrl_bits();

            let cr1 = self.cr1.read();
            let cr2 = self.cr2.read();
            let oar1 = self.oar1.read();
            let oar2 = self.oar2.read();
            let ccr = self.ccr.read();
            let trise = self.trise.read();
            let fltr = self.fltr.read();

            self.cr1.modify(|curr| curr | I2C_CR1_SWRST);
            self.cr1.modify(|curr| curr & !I2C_CR1_SWRST);

            self.cr2.write(cr2);
            self.oar1.write(oar1);
            self.oar2.write(oar2);
            self.ccr.write(ccr);
            self.trise.write(trise);
            self.fltr.write(fltr);
            self.cr1.write(cr1);
        }
    }

    fn clear_error_flags(&self) {
        // BERR - bus error - misplaced start or stop bit - Cleared by writing 0.
        // ARLO - arbitration lost - cleared by writing 0.
        // AF - acknowledge failure - set when no ACK received. Cleared by writing 0.
        unsafe {
            self.sr1
                .modify(|curr| curr & (!I2C_SR1_BERR & !I2C_SR1_ARLO & !I2C_SR1_AF));
        }
    }

    fn clear_transaction_ctrl_bits(&self) {
        unsafe {
            self.cr1.modify(|curr| {
                curr & (!I2C_CR1_POS & !I2C_CR1_ACK & !I2C_CR1_STOP & !I2C_CR1_START)
            });
        }
    }

    fn enable_periph(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_PE);
        }
    }

    fn disable_periph(&self) {
        unsafe {
            self.cr1.modify(|curr| curr & !I2C_CR1_PE);
        }
    }

    fn set_100mhz_standard_mode(&self, periph_freq_hz: u32) {
        unsafe {
            let ccr = periph_freq_hz / 200000;
            let trise = periph_freq_hz / 1000000 + 1;
            let periph_freq_mhz = periph_freq_hz / 1000000;

            self.cr2
                .modify(|curr| curr | (periph_freq_mhz & I2C_CR2_FREQ_MASK));
            self.ccr.write(ccr & I2C_CCR_CCR_MASK);
            self.trise.write(trise & I2C_TRISE_TRISE_MASK);
        }
    }

    fn set_analog_filter_state(&self, new_state: bool) {
        unsafe {
            self.fltr.modify(|curr| {
                if new_state {
                    curr & !I2C_FLTR_ANOFF
                } else {
                    curr | I2C_FLTR_ANOFF
                }
            });
        }
    }

    fn set_digital_filter_len(&self, new_len: u32) {
        unsafe {
            self.fltr
                .modify(|curr| curr | (new_len & I2C_FLTR_DNF_MASK));
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

    pub fn send_start(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_START);
        }
    }

    fn send_stop(&self) {
        unsafe { self.cr1.modify(|curr| curr | I2C_CR1_STOP) }
    }

    fn clear_addr_flag(&self) {
        self.sr1.read();
        self.sr2.read();
    }

    fn enable_event_interrupts(&self) {
        unsafe {
            self.cr2.modify(|curr| curr | I2C_CR2_ITEVTEN);
        }
    }

    fn enable_error_interrupts(&self) {
        unsafe {
            self.cr2.modify(|curr| curr | I2C_CR2_ITERREN);
        }
    }

    fn enable_buffer_interrupts(&self) {
        unsafe {
            self.cr2.modify(|curr| curr | I2C_CR2_ITBUFEN);
        }
    }

    fn disable_buffer_interrupts(&self) {
        unsafe {
            self.cr2.modify(|curr| curr & !I2C_CR2_ITBUFEN);
        }
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

    fn is_sb_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_SB != 0
    }

    fn send_addr(&self, addr: u8, dir: CommDir) {
        let dir_bit = match dir {
            CommDir::Read => 1,
            CommDir::Write => 0,
        };

        self.send_data((addr << 1) | dir_bit);
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
pub extern "C" fn rust_i2c_interrupt_handler(ctx_id: usize) {
    let ctx = unsafe { &mut I2C_PERIPHS[ctx_id] };

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
    let periph: &I2C = I2C::get_periph(ctx.periph_id);

    if let Some(ref mut trans) = ctx.current_transaction {
        match ctx.state {
            PeripheralState::StartBitSet => {
                if !periph.is_sb_set() {
                    return;
                }

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
                            periph.enable_buffer_interrupts();
                        } else if trans.steps[trans.curr_step].len() == 2 {
                            periph.disable_ack_after_next_byte();
                        } else {
                            periph.enable_ack();
                            periph.enable_buffer_interrupts();
                        }

                        periph.clear_addr_flag();

                        ctx.state = PeripheralState::Receiving(0);
                    }
                    Step::Write(ref data) => {
                        periph.clear_addr_flag();
                        periph.send_data(data[0]);

                        if data.len() >= 2 {
                            periph.enable_buffer_interrupts();
                        }

                        ctx.state = PeripheralState::Transmitting(1);
                    }
                }
            }

            PeripheralState::Transmitting(next_byte_pos) => {
                if (next_byte_pos >= trans.steps[trans.curr_step].len() && !periph.is_btf_set())
                    || !periph.is_txe_set()
                {
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

                    periph.disable_buffer_interrupts();
                } else {
                    periph.send_data(trans.steps[trans.curr_step].as_ref()[next_byte_pos]);
                    ctx.state = PeripheralState::Transmitting(next_byte_pos + 1);
                }
            }

            PeripheralState::Receiving(next_byte_pos) => {
                if next_byte_pos == trans.steps[trans.curr_step].len() - 1 {
                    if !periph.is_rxne_set() {
                        return;
                    }

                    trans.steps[trans.curr_step].as_mut()[next_byte_pos] = periph.read_data();

                    if trans.on_last_step() {
                        periph.disable_buffer_interrupts();
                        periph.send_stop();
                        ctx.state = PeripheralState::Done(0);
                    } else {
                        periph.send_start();
                        trans.curr_step += 1;
                        ctx.state = PeripheralState::StartBitSet;
                    }
                } else if next_byte_pos == trans.steps[trans.curr_step].len() - 2 {
                    if periph.is_rxne_set() {
                        periph.disable_ack();
                    }

                    if !periph.is_btf_set() {
                        return;
                    }

                    if trans.on_last_step() {
                        periph.send_stop();
                    }

                    trans.steps[trans.curr_step].as_mut()[next_byte_pos] = periph.read_data();
                    trans.steps[trans.curr_step].as_mut()[next_byte_pos + 1] = periph.read_data();

                    if trans.on_last_step() {
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

                    trans.steps[trans.curr_step].as_mut()[next_byte_pos] = periph.read_data();

                    ctx.state = PeripheralState::Receiving(next_byte_pos + 1);
                }
            }

            PeripheralState::Done(_) => {}
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_i2c_interrupt_error_handler(ctx_id: usize) {
    let ctx = unsafe { &mut I2C_PERIPHS[ctx_id] };

    let periph: &I2C = I2C::get_periph(ctx.periph_id);

    periph.clear_error_flags();
    periph.clear_transaction_ctrl_bits();

    ctx.state = PeripheralState::Done(-1);
}

pub static mut I2C_PERIPHS: [InterruptContext; 3] = [
    InterruptContext {
        current_transaction: None,
        periph_id: Peripheral::I2C1,
        state: PeripheralState::Done(0),
    },
    InterruptContext {
        current_transaction: None,
        periph_id: Peripheral::I2C2,
        state: PeripheralState::Done(0),
    },
    InterruptContext {
        current_transaction: None,
        periph_id: Peripheral::I2C3,
        state: PeripheralState::Done(0),
    },
];
