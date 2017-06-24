
use mouros_rust_bindings::CVoid;
use mouros_rust_bindings::tasks;

use i2c;

mod bmp085;

#[no_mangle]
pub extern "C" fn meteo_comm_loop(_params: *mut CVoid) {

    let mut bmp = bmp085::Bmp085::new(0);
    bmp.set_precision(bmp085::Precision::UltraHigh);

    loop {
        let _ = bmp.measure();

        tasks::sleep(1000);
    }
}
