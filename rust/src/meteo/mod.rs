
use mouros_rust_bindings::CVoid;
use mouros_rust_bindings::tasks;

use bsp::i2c;

mod bmp085;


#[no_mangle]
pub extern "C" fn rust_meteo_init() {}

#[no_mangle]
pub extern "C" fn rust_meteo_comm_loop(_params: *mut CVoid) {

    let mut bmp = bmp085::Bmp085::new(i2c::Peripheral::I2C3);
    bmp.set_precision(bmp085::Precision::UltraHigh);

    loop {
        let _ = bmp.measure();

        tasks::sleep(1000);
    }
}
