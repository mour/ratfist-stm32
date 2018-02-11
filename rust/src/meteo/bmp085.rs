use bsp::i2c;

use mouros::tasks;
use super::MeteoError;

const BMP085_I2C_ADDR: u8 = 0x77;

#[allow(dead_code)]
pub enum Precision {
    UltraLowPower,
    Standard,
    High,
    UltraHigh,
}

impl Precision {
    fn get_oversampling_param(&self) -> u32 {
        match *self {
            Precision::UltraLowPower => 0,
            Precision::Standard => 1,
            Precision::High => 2,
            Precision::UltraHigh => 3,
        }
    }

    fn get_pressure_conversion_time_ms(&self) -> u32 {
        match *self {
            Precision::UltraLowPower => 5,
            Precision::Standard => 8,
            Precision::High => 14,
            Precision::UltraHigh => 26,
        }
    }
}

struct CalibrationData {
    ac1: i32,
    ac2: i32,
    ac3: i32,
    ac4: u32,
    ac5: i32,
    ac6: i32,
    b1: i32,
    b2: i32,
    _mb: i32,
    mc: i32,
    md: i32,
}

pub struct Bmp085 {
    calib: Option<CalibrationData>,
    dev_addr: u8,
    i2c_bus: i2c::Peripheral,
    precision: Precision,
}

impl Bmp085 {
    pub fn new(periph: i2c::Peripheral) -> Bmp085 {
        Bmp085 {
            calib: None,
            dev_addr: BMP085_I2C_ADDR,
            i2c_bus: periph,
            precision: Precision::Standard,
        }
    }

    fn get_calibration_data(&mut self) -> Result<(), MeteoError> {
        let mut set_addr = [0xaa];
        let mut conf_data = [0; 22];

        self.i2c_bus
            .run_transaction(
                self.dev_addr,
                &mut [
                    i2c::Step::Write(&mut set_addr),
                    i2c::Step::Read(&mut conf_data),
                ],
            )
            .map_err(|_| MeteoError::I2CCommError)?;

        self.calib = Some(CalibrationData {
            ac1: ((conf_data[0] as u16) << 8 | conf_data[1] as u16) as i16 as i32,
            ac2: ((conf_data[2] as u16) << 8 | conf_data[3] as u16) as i16 as i32,
            ac3: ((conf_data[4] as u16) << 8 | conf_data[5] as u16) as i16 as i32,
            ac4: (conf_data[6] as u32) << 8 | conf_data[7] as u32,
            ac5: ((conf_data[8] as u16) << 8 | conf_data[9] as u16) as i16 as i32,
            ac6: ((conf_data[10] as u16) << 8 | conf_data[11] as u16) as i16 as i32,
            b1: ((conf_data[12] as u16) << 8 | conf_data[13] as u16) as i16 as i32,
            b2: ((conf_data[14] as u16) << 8 | conf_data[15] as u16) as i16 as i32,
            _mb: ((conf_data[16] as u16) << 8 | conf_data[17] as u16) as i16 as i32,
            mc: ((conf_data[18] as u16) << 8 | conf_data[19] as u16) as i16 as i32,
            md: ((conf_data[20] as u16) << 8 | conf_data[21] as u16) as i16 as i32,
        });

        Ok(())
    }

    fn get_raw_temp_val(&self) -> Result<i32, MeteoError> {
        let mut t_meas_cmd = [0xf4, 0x2e];
        let mut t_data_addr = [0xf6];
        let mut t_data_buf = [0; 2];

        self.i2c_bus
            .run_transaction(self.dev_addr, &mut [i2c::Step::Write(&mut t_meas_cmd)])
            .map_err(|_| MeteoError::I2CCommError)?;

        tasks::sleep(5);

        self.i2c_bus
            .run_transaction(
                self.dev_addr,
                &mut [
                    i2c::Step::Write(&mut t_data_addr),
                    i2c::Step::Read(&mut t_data_buf),
                ],
            )
            .map_err(|_| MeteoError::I2CCommError)?;

        Ok((((t_data_buf[0] as u32) << 8) | t_data_buf[1] as u32) as i32)
    }

    fn get_raw_pressure_val(&self) -> Result<i32, MeteoError> {
        let oss = self.precision.get_oversampling_param();

        let mut p_meas_cmd = [0xf4, (0x34 + (oss << 6)) as u8];
        let mut p_data_addr = [0xf6];
        let mut p_data_buf = [0; 3];

        self.i2c_bus
            .run_transaction(self.dev_addr, &mut [i2c::Step::Write(&mut p_meas_cmd)])
            .map_err(|_| MeteoError::I2CCommError)?;

        tasks::sleep(self.precision.get_pressure_conversion_time_ms());

        self.i2c_bus
            .run_transaction(
                self.dev_addr,
                &mut [
                    i2c::Step::Write(&mut p_data_addr),
                    i2c::Step::Read(&mut p_data_buf),
                ],
            )
            .map_err(|_| MeteoError::I2CCommError)?;

        Ok(
            (((p_data_buf[0] as u32) << 16 | (p_data_buf[1] as u32) << 8 | p_data_buf[2] as u32)
                >> (8 - oss)) as i32,
        )
    }

    pub fn set_precision(&mut self, precision: Precision) {
        self.precision = precision;
    }

    pub fn measure(&mut self) -> Result<(f32, f32), MeteoError> {
        if self.calib.is_none() {
            self.get_calibration_data()?;
        }

        let calib = self.calib.as_ref().ok_or(MeteoError::I2CCommError)?;

        let ut = self.get_raw_temp_val()?;
        let up = self.get_raw_pressure_val()?;

        let x1: i32 = (ut - calib.ac6) * calib.ac5 >> 15;
        let x2: i32 = (calib.mc << 11) / (x1 + calib.md);
        let b5: i32 = x1 + x2;

        let t: i32 = (b5 + 8) >> 4;

        let b6: i32 = b5 - 4000;
        let x1: i32 = (calib.b2 * ((b6 * b6) >> 12)) >> 11;
        let x2: i32 = calib.ac2 * b6 >> 11;
        let x3: i32 = x1 + x2;

        let oss = self.precision.get_oversampling_param();

        let b3: i32 = ((((calib.ac1 << 2) + x3) << oss) + 2) >> 2;
        let x1: i32 = calib.ac3 * b6 >> 13;
        let x2: i32 = (calib.b1 * ((b6 * b6) >> 12)) >> 16;
        let x3: i32 = ((x1 + x2) + 2) >> 2;
        let b4: u32 = calib.ac4 * ((x3 + 32768) as u32) >> 15;
        let b7: u32 = ((up - b3) as u32) * (50000 >> oss);

        let p1: i32 = if b7 < 0x80000000 {
            ((b7 << 1) / b4) as i32
        } else {
            ((b7 / b4) << 1) as i32
        };

        let x1: i32 = (p1 >> 8) * (p1 >> 8);
        let x1: i32 = (x1 * 3038) >> 16;
        let x2: i32 = (-7357 * p1) >> 16;

        let p: i32 = p1 + ((x1 + x2 + 3791) >> 4);

        Ok(((t as f32) / 10.0, p as f32))
    }
}
