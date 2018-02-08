
use bsp::i2c;

const TSL2561_I2C_ADDR: u8 = 0x29;

const CONTROL_REG_ADDR: u8 = 0x00 | 0x80 | 0x10;
const TIMING_REG_ADDR: u8 = 0x01 | 0x80 | 0x10;
const DATA_REG_ADDR: u8 = 0x0b | 0x80 | 0x10;
const DATA_BYTE_NUM: usize = 5;


#[derive(Clone, Copy)]
#[repr(u8)]
pub enum State {
    On = 0x03,
    Off = 0x00,
}

#[derive(Clone, Copy)]
#[repr(u8)]
pub enum Gain {
    Gain1x = 0x10,
    Gain16x = 0x00,
}

impl Gain {
    fn get_scaling_factor(&self) -> f32 {
        match *self {
            Gain::Gain1x => 1.0,
            Gain::Gain16x => 16.0,
        }
    }
}

#[allow(non_camel_case_types)]
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum IntegrationTime {
    Time13_7ms = 0x00,
    Time101ms = 0x01,
    Time402ms = 0x10,
}

impl IntegrationTime {
    fn get_scaling_factor(&self) -> f32 {
        match *self {
            IntegrationTime::Time13_7ms => 0.034,
            IntegrationTime::Time101ms => 0.252,
            IntegrationTime::Time402ms => 1.0,
        }
    }
}

pub struct Tsl2561 {
    dev_addr: u8,
    i2c_bus: i2c::Peripheral,
    gain: Gain,
    integ_time: IntegrationTime,
    state: State,
}

impl Tsl2561 {
    pub fn new(periph: i2c::Peripheral) -> Tsl2561 {
        Tsl2561 {
            dev_addr: TSL2561_I2C_ADDR,
            i2c_bus: periph,
            gain: Gain::Gain1x,
            integ_time: IntegrationTime::Time402ms,
            state: State::Off,
        }
    }

    pub fn set_state(&mut self, new_state: State) -> Result<(), ()> {
        let mut state_cmd = [CONTROL_REG_ADDR, 0x01, new_state as u8];

        self.i2c_bus
            .run_transaction(TSL2561_I2C_ADDR, &mut [i2c::Step::Write(&mut state_cmd)])
            .and_then(|_| {
                self.state = new_state;
                Ok(())
            })
    }

    pub fn set_gain_and_integ_time(
        &mut self,
        new_gain: Gain,
        new_integ_time: IntegrationTime,
    ) -> Result<(), ()> {
        let mut g_it_cmd = [TIMING_REG_ADDR, 0x01, (new_gain as u8) | (new_integ_time as u8)];

        self.i2c_bus
            .run_transaction(TSL2561_I2C_ADDR, &mut [i2c::Step::Write(&mut g_it_cmd)])
            .and_then(|_| {
                self.gain = new_gain;
                self.integ_time = new_integ_time;
                Ok(())
            })
    }

    fn get_raw_data(&self) -> Result<(u16, u16), ()> {
        let mut data_addr_cmd = [DATA_REG_ADDR];
        let mut data_buf = [0; DATA_BYTE_NUM];

        let res = self.i2c_bus.run_transaction(
            TSL2561_I2C_ADDR,
            &mut [
                i2c::Step::Write(&mut data_addr_cmd),
                i2c::Step::Read(&mut data_buf),
            ],
        );

        res.map(|_| {
            (
                ((data_buf[2] as u16) << 8) | (data_buf[1] as u16),
                ((data_buf[4] as u16) << 8) | (data_buf[3] as u16),
            )
        })
    }

    pub fn measure(&mut self) -> Result<f32, ()> {
        let (raw_ch0, raw_ch1) = self.get_raw_data()?;
        if raw_ch0 == 0 {
            return Ok(0.0);
        }

        let ch0 = (raw_ch0 as f32) / self.gain.get_scaling_factor() /
            self.integ_time.get_scaling_factor();

        let ch1 = (raw_ch1 as f32) / self.gain.get_scaling_factor() /
            self.integ_time.get_scaling_factor();


        match ch1 / ch0 {
            x if 0.0 <= x && x < 0.125 => Ok(0.0304 * ch0 - 0.0272 * ch1),
            x if 0.125 <= x && x < 0.25 => Ok(0.0325 * ch0 - 0.0440 * ch1),
            x if 0.25 <= x && x < 0.375 => Ok(0.0351 * ch0 - 0.0544 * ch1),
            x if 0.375 <= x && x < 0.50 => Ok(0.0381 * ch0 - 0.0624 * ch1),
            x if 0.50 <= x && x < 0.61 => Ok(0.0224 * ch0 - 0.0310 * ch1),
            x if 0.61 <= x && x < 0.80 => Ok(0.0128 * ch0 - 0.0153 * ch1),
            x if 0.80 <= x && x < 1.30 => Ok(0.00146 * ch0 - 0.00112 * ch1),
            x if 1.30 <= x => Ok(0.0),
            _ => Err(()),
        }
    }
}
