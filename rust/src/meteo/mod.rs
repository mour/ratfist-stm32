
use mouros_rust_bindings::CVoid;

use bsp::i2c;

#[no_mangle]
pub extern "C" fn meteo_comm_loop(_params: *mut CVoid) {
    
    let mut periph = i2c::I2C::new(i2c::Peripherals::I2C1);
    
    let mut write1 = [0u8; 10];    
    let mut read2 = [0u8; 4];
    
    let mut steps = [i2c::Step::Write(&mut write1), i2c::Step::Read(&mut read2)];
    
    loop {
        let _ = periph.run_transaction(0xaa, &mut steps);
    }
}