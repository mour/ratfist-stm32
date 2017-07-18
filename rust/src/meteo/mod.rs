
use mouros_rust_bindings::CVoid;
use mouros_rust_bindings::tasks;

use bsp::i2c;

#[no_mangle]
pub extern "C" fn rust_meteo_init() {}

#[no_mangle]
pub extern "C" fn rust_meteo_comm_loop(_params: *mut CVoid) {

    let mut periph = i2c::I2C::new(i2c::Peripheral::I2C3);

    let mut write1 = [0x20];
    let mut read2 = [0u8];

    let mut steps = [i2c::Step::Write(&mut write1), i2c::Step::Read(&mut read2)];

    loop {
        let _ = periph.run_transaction(0x19, &mut steps);
        tasks::sleep(1000);
    }
}
