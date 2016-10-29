
extern crate mouros_rust_bindings;

use self::mouros_rust_bindings::tasks;
use self::mouros_rust_bindings::CVoid;

use bindings::bsp;

state_machine!(spinner_machine;
        Stopped;
        [Stopped, Spinning, Spindown];
        [SetPlan, GetPlan, SetState, GetState];
        [
            Stopped => [
                [SetPlan] Stopped | {0},
                [GetPlan] Stopped | {0}
            ]
        ]
);

#[no_mangle]
pub extern fn spinner_main_loop(_params: *mut CVoid) {
    unsafe {
        bsp::bsp_led_toggle(bsp::board_led::LED2);

        tasks::os_task_sleep(1000);
    }
}