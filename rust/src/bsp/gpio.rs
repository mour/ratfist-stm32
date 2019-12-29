use crate::bsp;
use volatile_register::{RO, RW, WO};

const GPIO_MODER_MODER_INPUT: u32 = 0;
const GPIO_MODER_MODER_OUTPUT: u32 = 1;
const GPIO_MODER_MODER_AF: u32 = 2;
const GPIO_MODER_MODER_ANALOG: u32 = 3;

const GPIO_OTYPER_OT_PUSHPULL: u32 = 0;
const GPIO_OTYPER_OT_OPENDRAIN: u32 = 1;

const GPIO_OSPEEDR_OSPEEDR_LOW: u32 = 0;
const GPIO_OSPEEDR_OSPEEDR_MEDIUM: u32 = 1;
const GPIO_OSPEEDR_OSPEEDR_FAST: u32 = 2;
const GPIO_OSPEEDR_OSPEEDR_HIGH: u32 = 3;

const GPIO_PUPDR_PUPDR_FLOATING: u32 = 0;
const GPIO_PUPDR_PUPDR_PULLUP: u32 = 1;
const GPIO_PUPDR_PUPDR_PULLDOWN: u32 = 2;

const GPIO_AFR_AFR_AF0: u32 = 0;
const GPIO_AFR_AFR_AF1: u32 = 1;
const GPIO_AFR_AFR_AF2: u32 = 2;
const GPIO_AFR_AFR_AF3: u32 = 3;
const GPIO_AFR_AFR_AF4: u32 = 4;
const GPIO_AFR_AFR_AF5: u32 = 5;
const GPIO_AFR_AFR_AF6: u32 = 6;
const GPIO_AFR_AFR_AF7: u32 = 7;
const GPIO_AFR_AFR_AF8: u32 = 8;
const GPIO_AFR_AFR_AF9: u32 = 9;
const GPIO_AFR_AFR_AF10: u32 = 10;
const GPIO_AFR_AFR_AF11: u32 = 11;
const GPIO_AFR_AFR_AF12: u32 = 12;
const GPIO_AFR_AFR_AF13: u32 = 13;
const GPIO_AFR_AFR_AF14: u32 = 14;
const GPIO_AFR_AFR_AF15: u32 = 15;

struct PortRegs {
    moder: RW<u32>,
    otyper: RW<u32>,
    ospeedr: RW<u32>,
    pupdr: RW<u32>,
    idr: RO<u32>,
    odr: RW<u32>,
    bsrr: WO<u32>,
    _lckr: RW<u32>,
    afrl: RW<u32>,
    afrh: RW<u32>,
}

#[derive(Clone, Copy)]
pub enum Gpio {
    GpioA,
    GpioB,
    GpioC,
}

#[derive(Clone, Copy)]
pub enum Mode {
    Input,
    Output(OutputType, OutputSpeed),
    AlternateFunction(AltFuncNum, OutputType, OutputSpeed),
    Analog,
}

#[repr(u32)]
#[derive(Clone, Copy)]
pub enum OutputType {
    PushPull = GPIO_OTYPER_OT_PUSHPULL,
    OpenDrain = GPIO_OTYPER_OT_OPENDRAIN,
}

#[repr(u32)]
#[derive(Clone, Copy)]
pub enum OutputSpeed {
    Low = GPIO_OSPEEDR_OSPEEDR_LOW,
    Medium = GPIO_OSPEEDR_OSPEEDR_MEDIUM,
    Fast = GPIO_OSPEEDR_OSPEEDR_FAST,
    High = GPIO_OSPEEDR_OSPEEDR_HIGH,
}

#[repr(u32)]
#[derive(Clone, Copy)]
pub enum PullUpDown {
    None = GPIO_PUPDR_PUPDR_FLOATING,
    PullUp = GPIO_PUPDR_PUPDR_PULLUP,
    PullDown = GPIO_PUPDR_PUPDR_PULLDOWN,
}

#[repr(u32)]
#[derive(Clone, Copy)]
pub enum AltFuncNum {
    Af0 = GPIO_AFR_AFR_AF0,
    Af1 = GPIO_AFR_AFR_AF1,
    Af2 = GPIO_AFR_AFR_AF2,
    Af3 = GPIO_AFR_AFR_AF3,
    Af4 = GPIO_AFR_AFR_AF4,
    Af5 = GPIO_AFR_AFR_AF5,
    Af6 = GPIO_AFR_AFR_AF6,
    Af7 = GPIO_AFR_AFR_AF7,
    Af8 = GPIO_AFR_AFR_AF8,
    Af9 = GPIO_AFR_AFR_AF9,
    Af10 = GPIO_AFR_AFR_AF10,
    Af11 = GPIO_AFR_AFR_AF11,
    Af12 = GPIO_AFR_AFR_AF12,
    Af13 = GPIO_AFR_AFR_AF13,
    Af14 = GPIO_AFR_AFR_AF14,
    Af15 = GPIO_AFR_AFR_AF15,
}

#[repr(u8)]
#[derive(Clone, Copy)]
pub enum PinNum {
    Pin0 = 0,
    Pin1 = 1,
    Pin2 = 2,
    Pin3 = 3,
    Pin4 = 4,
    Pin5 = 5,
    Pin6 = 6,
    Pin7 = 7,
    Pin8 = 8,
    Pin9 = 9,
    Pin10 = 10,
    Pin11 = 11,
    Pin12 = 12,
    Pin13 = 13,
    Pin14 = 14,
    Pin15 = 15,
}

#[derive(Clone, Copy)]
pub struct Pin {
    port: &'static PortRegs,
    pin_num: u8,
}

impl Pin {
    pub fn new(port: Gpio, pin_num: PinNum) -> Pin {
        let base_addr = match port {
            Gpio::GpioA => bsp::periph_base_addr::GPIOA,
            Gpio::GpioB => bsp::periph_base_addr::GPIOB,
            Gpio::GpioC => bsp::periph_base_addr::GPIOC,
        };

        Pin {
            port: unsafe { &*(base_addr as *const PortRegs) },
            pin_num: pin_num as u8,
        }
    }

    pub fn set_mode(&self, mode: Mode) {
        unsafe {
            self.port.moder.modify(|curr| {
                let mask = 0x3 << (2 * self.pin_num);
                let new_mode = match mode {
                    Mode::Input => GPIO_MODER_MODER_INPUT,
                    Mode::Output(_, _) => GPIO_MODER_MODER_OUTPUT,
                    Mode::AlternateFunction(_, _, _) => GPIO_MODER_MODER_AF,
                    Mode::Analog => GPIO_MODER_MODER_ANALOG,
                } << (2 * self.pin_num);

                (curr & !mask) | new_mode
            });
        }

        match mode {
            Mode::Output(otype, ospeed) => {
                self.set_output_type(otype);
                self.set_output_speed(ospeed);
            }
            Mode::AlternateFunction(af_num, otype, ospeed) => {
                self.set_alternate_function(af_num);
                self.set_output_type(otype);
                self.set_output_speed(ospeed);
            }
            _ => {}
        }
    }

    pub fn set_pullup_pulldown_mode(&self, mode: PullUpDown) {
        unsafe {
            self.port.pupdr.modify(|curr| {
                let mask = 0x3 << (2 * self.pin_num);
                let new_type = (mode as u32) << (2 * self.pin_num);

                (curr & !mask) | new_type
            });
        }
    }

    pub fn read(&self) -> bool {
        (self.port.idr.read() & (1 << self.pin_num)) != 0
    }

    pub fn set(&self) {
        unsafe {
            self.port.bsrr.write(1 << self.pin_num);
        }
    }

    pub fn reset(&self) {
        unsafe {
            self.port.bsrr.write(0x00010000 << self.pin_num);
        }
    }

    pub fn toggle(&self) {
        if (self.port.odr.read() & (1 << self.pin_num)) != 0 {
            self.reset();
        } else {
            self.set();
        }
    }

    fn set_output_type(&self, otype: OutputType) {
        unsafe {
            self.port.otyper.modify(|curr| {
                let mask = 1 << self.pin_num;
                let new_type = (otype as u32) << self.pin_num;

                (curr & !mask) | new_type
            });
        }
    }

    fn set_output_speed(&self, ospeed: OutputSpeed) {
        unsafe {
            self.port.ospeedr.modify(|curr| {
                let mask = 0x3 << (2 * self.pin_num);
                let new_speed = (ospeed as u32) << (2 * self.pin_num);

                (curr & !mask) | new_speed
            });
        }
    }

    fn set_alternate_function(&self, af: AltFuncNum) {
        let reg = if self.pin_num < 8 {
            &self.port.afrl
        } else {
            &self.port.afrh
        };

        unsafe {
            reg.modify(|curr| {
                let offset = (self.pin_num % 8) * 4;
                let mask = 0xf << offset;
                let new_af_num = (af as u32) << offset;

                (curr & !mask) | new_af_num
            });
        }
    }
}
