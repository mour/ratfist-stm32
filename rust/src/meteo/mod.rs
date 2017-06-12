
use mouros_rust_bindings::CVoid;

use i2c;

#[no_mangle]
pub extern "C" fn meteo_comm_loop(_params: *mut CVoid) {
    
    let mut write1 = [0u8; 10];    
    let mut read2 = [0u8; 4];
    
    let mut steps = [i2c::Step::Write(&mut write1), i2c::Step::Read(&mut read2)];
    
    i2c::start_transaction(0xaa, &mut steps);
}